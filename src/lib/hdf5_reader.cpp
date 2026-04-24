#include "fastscope/hdf5_reader.hpp"
#include "fastscope/log.hpp"

#include <hdf5.h>

#include <cassert>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace fastscope {

// ── Impl ───────────────────────────────────────────────────────────────────────

struct HDF5Reader::Impl {
    hid_t               file_id = H5I_INVALID_HID;
    std::filesystem::path path;
};

// ── Helper: HDF5 type → DataType ──────────────────────────────────────────────

static DataType hdf5_to_dtype(hid_t type_id) noexcept
{
    const H5T_class_t cls   = H5Tget_class(type_id);
    const std::size_t sz    = H5Tget_size(type_id);
    const H5T_sign_t  sign  = (cls == H5T_INTEGER) ? H5Tget_sign(type_id) : H5T_SGN_NONE;

    if (cls == H5T_FLOAT) {
        return sz == 4 ? DataType::Float32 : DataType::Float64;
    }
    if (cls == H5T_INTEGER) {
        if (sign == H5T_SGN_NONE) {
            return sz == 4 ? DataType::UInt32 : DataType::UInt64;
        } else {
            return sz == 4 ? DataType::Int32 : DataType::Int64;
        }
    }
    return DataType::Unknown;
}

// ── Helper: read a 1-D (or Nx1) dataset as vector<double> ────────────────────
// Accepts shapes (N,) and (N, 1) — both occur in real simulation files.

static fastscope::Result<std::vector<double>>
read_1d_doubles(hid_t file_id, const std::string& ds_path)
{
    const hid_t ds = H5Dopen2(file_id, ds_path.c_str(), H5P_DEFAULT);
    if (ds < 0)
        return fastscope::make_error<std::vector<double>>(
            fastscope::ErrorCode::NotFound,
            "HDF5 dataset not found: " + ds_path);

    hid_t space = H5Dget_space(ds);
    int   ndims = H5Sget_simple_extent_ndims(space);
    hsize_t dims[2] = {0, 1};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    H5Sclose(space);

    // Accept (N,) and (N, 1)
    const bool ok = (ndims == 1) || (ndims == 2 && dims[1] == 1);
    if (!ok) {
        H5Dclose(ds);
        return fastscope::make_error<std::vector<double>>(
            fastscope::ErrorCode::InvalidArgument,
            ds_path + " is not 1-D (shape must be (N,) or (N,1))");
    }

    std::vector<double> out(dims[0]);
    const herr_t err = H5Dread(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, out.data());
    H5Dclose(ds);
    if (err < 0)
        return fastscope::make_error<std::vector<double>>(
            fastscope::ErrorCode::IOError,
            "H5Dread failed for " + ds_path);
    return out;
}

// ── Helper: read string attribute ─────────────────────────────────────────────

static std::string read_string_attr(hid_t obj_id, const char* attr_name)
{
    if (!H5Aexists(obj_id, attr_name)) return {};

    const hid_t attr  = H5Aopen(obj_id, attr_name, H5P_DEFAULT);
    if (attr < 0) return {};

    hid_t type = H5Aget_type(attr);
    const std::size_t sz = H5Tget_size(type);

    std::string buf(sz + 1, '\0');
    const herr_t err = H5Aread(attr, type, buf.data());
    H5Tclose(type);
    H5Aclose(attr);

    if (err < 0) return {};
    buf.resize(std::strlen(buf.c_str()));  // trim null bytes
    return buf;
}

// ── Enumerate visitor ─────────────────────────────────────────────────────────
// Enumerates ALL datasets — no filtering. The caller decides what is a time
// axis and what is a signal.

struct EnumContext {
    hid_t                     file_id;
    std::vector<SignalMetadata>* out;
    std::string               sim_id;
};

static herr_t enum_visitor(hid_t /*loc_id*/, const char* name,
                            const H5L_info2_t* /*linfo*/, void* opaque)
{
    auto* ctx = static_cast<EnumContext*>(opaque);

    const hid_t ds = H5Dopen2(ctx->file_id, name, H5P_DEFAULT);
    if (ds < 0) return 0;   // not a dataset (e.g. group) → skip

    hid_t space = H5Dget_space(ds);
    hid_t dtype = H5Dget_type(ds);

    const int ndims = H5Sget_simple_extent_ndims(space);
    if (ndims < 1 || ndims > 2) {
        H5Tclose(dtype); H5Sclose(space); H5Dclose(ds);
        return 0;
    }

    hsize_t dims[2] = {0, 0};
    H5Sget_simple_extent_dims(space, dims, nullptr);

    SignalMetadata meta;
    meta.sim_id  = ctx->sim_id;
    meta.h5_path = std::string("/") + name;  // ensure leading slash

    // Display name = last path component
    std::string s(name);
    meta.name = s.substr(s.rfind('/') + 1);

    meta.dtype = hdf5_to_dtype(dtype);
    meta.shape.push_back(static_cast<std::size_t>(dims[0]));
    if (ndims == 2)
        meta.shape.push_back(static_cast<std::size_t>(dims[1]));

    meta.units = read_string_attr(ds, "units");
    // time_path intentionally left empty — set by caller after user selects it

    H5Tclose(dtype);
    H5Sclose(space);
    H5Dclose(ds);

    ctx->out->push_back(std::move(meta));
    return 0;
}

// ── suggest_time_axes visitor ─────────────────────────────────────────────────

struct TimeHintContext {
    hid_t                    file_id;
    std::vector<std::string>* candidates; // ordered best-first
};

static bool name_looks_like_time(const std::string& name) noexcept
{
    // case-insensitive check for the leaf name
    std::string leaf = name.substr(name.rfind('/') + 1);
    for (char& c : leaf) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return leaf == "t" || leaf == "time" ||
           leaf.find("time") != std::string::npos;
}

// ── find_time_in_group: search immediate children of one group ────────────────

struct GroupTimeCtx {
    std::string group_path;          // absolute path of the group being searched
    std::optional<std::string> found; // first matching time dataset, full path
};

static herr_t group_time_member(hid_t group_id, const char* name,
                                 const H5L_info2_t* /*info*/, void* opaque)
{
    auto* ctx = static_cast<GroupTimeCtx*>(opaque);
    if (ctx->found) return 1;  // already found — stop iteration

    const hid_t ds = H5Dopen2(group_id, name, H5P_DEFAULT);
    if (ds < 0) return 0;  // not a dataset (sub-group) — skip

    hid_t space = H5Dget_space(ds);
    hid_t dtype = H5Dget_type(ds);
    const int ndims = H5Sget_simple_extent_ndims(space);
    hsize_t dims[2] = {0, 1};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    const H5T_class_t cls = H5Tget_class(dtype);
    H5Tclose(dtype);
    H5Sclose(space);
    H5Dclose(ds);

    const bool is_1d_float = (cls == H5T_FLOAT) &&
                              ((ndims == 1) || (ndims == 2 && dims[1] == 1));
    if (is_1d_float && name_looks_like_time(name)) {
        ctx->found = (ctx->group_path == "/")
                   ? std::string("/") + name
                   : ctx->group_path + "/" + name;
        return 1;
    }
    return 0;
}

/// Walk up the HDF5 hierarchy from @p signal_path to find the nearest time dataset.
/// Returns the full HDF5 path of the first match, or std::nullopt if none found.
static std::optional<std::string>
find_time_path_for_impl(hid_t file_id, const std::string& signal_path)
{
    std::string current = signal_path;

    while (true) {
        // Compute parent group path
        const auto slash = current.rfind('/');
        std::string group = (slash == std::string::npos || slash == 0)
                          ? "/" : current.substr(0, slash);

        // Search direct children of this group
        const hid_t grp = H5Gopen2(file_id, group.c_str(), H5P_DEFAULT);
        if (grp >= 0) {
            GroupTimeCtx ctx{group, std::nullopt};
            hsize_t idx = 0;
            H5Literate2(grp, H5_INDEX_NAME, H5_ITER_NATIVE, &idx,
                        group_time_member, &ctx);
            H5Gclose(grp);
            if (ctx.found) return ctx.found;
        }

        if (group == "/") break;
        current = group;
    }

    return std::nullopt;
}

static herr_t time_hint_visitor(hid_t /*loc_id*/, const char* name,
                                 const H5L_info2_t* /*linfo*/, void* opaque)
{
    auto* ctx = static_cast<TimeHintContext*>(opaque);

    const hid_t ds = H5Dopen2(ctx->file_id, name, H5P_DEFAULT);
    if (ds < 0) return 0;

    hid_t space = H5Dget_space(ds);
    hid_t dtype = H5Dget_type(ds);
    const int ndims = H5Sget_simple_extent_ndims(space);
    hsize_t dims[2] = {0, 1};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    const H5T_class_t cls = H5Tget_class(dtype);

    H5Tclose(dtype); H5Sclose(space); H5Dclose(ds);

    // Must be float, 1-D or (N,1), and have a time-like name
    const bool is_float = (cls == H5T_FLOAT);
    const bool right_shape = (ndims == 1) || (ndims == 2 && dims[1] == 1);
    if (is_float && right_shape && name_looks_like_time(name))
        ctx->candidates->push_back(std::string("/") + name);

    return 0;
}

// ── HDF5Reader implementation ─────────────────────────────────────────────────

HDF5Reader::HDF5Reader() : m_impl(std::make_unique<Impl>()) {}
HDF5Reader::~HDF5Reader() { close(); }

HDF5Reader::HDF5Reader(HDF5Reader&&) noexcept = default;
HDF5Reader& HDF5Reader::operator=(HDF5Reader&&) noexcept = default;

fastscope::VoidResult HDF5Reader::open(const std::filesystem::path& path)
{
    if (is_open()) close();

    if (!std::filesystem::exists(path))
        return fastscope::make_void_error(fastscope::ErrorCode::NotFound,
                                    "File not found: " + path.string());

    // Suppress HDF5 error printing to stderr
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    const hid_t fid = H5Fopen(path.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (fid < 0)
        return fastscope::make_void_error(fastscope::ErrorCode::IOError,
                                    "H5Fopen failed: " + path.string());

    m_impl->file_id = fid;
    m_impl->path    = path;
    FASTSCOPE_LOG_INFO("HDF5: opened {}", path.string());
    return {};
}

void HDF5Reader::close() noexcept
{
    if (m_impl && m_impl->file_id != H5I_INVALID_HID) {
        H5Fclose(m_impl->file_id);
        m_impl->file_id = H5I_INVALID_HID;
    }
}

bool HDF5Reader::is_open() const noexcept
{
    return m_impl && m_impl->file_id != H5I_INVALID_HID;
}

const std::filesystem::path& HDF5Reader::path() const noexcept
{
    static const std::filesystem::path empty;
    return m_impl ? m_impl->path : empty;
}

fastscope::Result<std::vector<SignalMetadata>>
HDF5Reader::enumerate_signals(const std::string& sim_id) const
{
    if (!is_open())
        return fastscope::make_error<std::vector<SignalMetadata>>(
            fastscope::ErrorCode::InvalidState, "File not open");

    std::vector<SignalMetadata> out;
    EnumContext ctx{m_impl->file_id, &out, sim_id};

    const herr_t err = H5Lvisit2(m_impl->file_id, H5_INDEX_NAME,
                                   H5_ITER_NATIVE, enum_visitor, &ctx);
    if (err < 0)
        return fastscope::make_error<std::vector<SignalMetadata>>(
            fastscope::ErrorCode::IOError, "H5Lvisit2 failed");

    // ── Auto-discover time axes (cached by parent group) ──────────────────────
    // Signals in the same parent group share the same time resolution walk,
    // so we cache results keyed by parent group path to avoid redundant scans.
    std::unordered_map<std::string, std::string> time_cache;   // group → time_path

    for (auto& meta : out) {
        // Parent group of this signal
        const auto slash  = meta.h5_path.rfind('/');
        const std::string group = (slash == std::string::npos || slash == 0)
                                ? "/" : meta.h5_path.substr(0, slash);

        auto it = time_cache.find(group);
        if (it == time_cache.end()) {
            auto tp = find_time_path_for_impl(m_impl->file_id, meta.h5_path);
            const std::string tp_str = tp.value_or("");
            time_cache[group] = tp_str;
            meta.time_path = tp_str;
        } else {
            meta.time_path = it->second;
        }
    }

    FASTSCOPE_LOG_DEBUG("HDF5: enumerated {} datasets in {}", out.size(), m_impl->path.string());
    return out;
}

std::vector<std::string> HDF5Reader::suggest_time_axes() const
{
    if (!is_open()) return {};

    std::vector<std::string> candidates;
    TimeHintContext ctx{m_impl->file_id, &candidates};
    H5Lvisit2(m_impl->file_id, H5_INDEX_NAME, H5_ITER_NATIVE,
               time_hint_visitor, &ctx);
    return candidates;
}

fastscope::Result<std::shared_ptr<SignalBuffer>>
HDF5Reader::load_signal(const SignalMetadata& meta) const
{
    if (!is_open())
        return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
            fastscope::ErrorCode::InvalidState, "File not open");

    // Open signal dataset
    const hid_t ds = H5Dopen2(m_impl->file_id, meta.h5_path.c_str(), H5P_DEFAULT);
    if (ds < 0)
        return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
            fastscope::ErrorCode::NotFound,
            "Dataset not found: " + meta.h5_path);

    hid_t space = H5Dget_space(ds);
    int   ndims = H5Sget_simple_extent_ndims(space);
    hsize_t dims[2] = {0, 1};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    H5Sclose(space);

    const std::size_t N = static_cast<std::size_t>(dims[0]);

    // Shape normalisation:
    //   (N,)   → 1 component
    //   (N, 1) → 1 component  (squeeze trailing singleton)
    //   (N, K) → K components
    const std::size_t n_comp = (ndims == 2 && dims[1] > 1)
                                 ? static_cast<std::size_t>(dims[1])
                                 : 1u;

    std::vector<double> values(N * n_comp);
    const herr_t err = H5Dread(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, values.data());
    H5Dclose(ds);
    if (err < 0)
        return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
            fastscope::ErrorCode::IOError,
            "H5Dread failed for " + meta.h5_path);

    // Time axis: use meta.time_path if set; otherwise error
    std::vector<double> time_vec;
    if (!meta.time_path.empty()) {
        auto time_res = read_1d_doubles(m_impl->file_id, meta.time_path);
        if (!time_res)
            return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
                time_res.error().code,
                "Time axis '" + meta.time_path + "': " + time_res.error().message);
        time_vec = std::move(*time_res);
    } else {
        // No time reference was found during enumeration — refuse to plot
        return fastscope::make_error<std::shared_ptr<SignalBuffer>>(
            fastscope::ErrorCode::NotFound,
            "No time reference found for '" + meta.h5_path + "'. "
            "FastScope searched up the HDF5 hierarchy from this signal's group "
            "but found no dataset named 'time' or 't'. "
            "The signal cannot be plotted without a time axis.");
    }

    auto buf = SignalBuffer::make_vector(
        meta, std::move(time_vec), std::move(values), n_comp);
    return buf;
}

} // namespace fastscope
