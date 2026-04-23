#include "gnc_viz/hdf5_reader.hpp"
#include "gnc_viz/log.hpp"

#include <hdf5.h>

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace gnc_viz {

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

// ── Helper: read 1-D dataset as vector<double> ────────────────────────────────

static gnc::Result<std::vector<double>>
read_1d_doubles(hid_t file_id, const std::string& ds_path)
{
    const hid_t ds = H5Dopen2(file_id, ds_path.c_str(), H5P_DEFAULT);
    if (ds < 0)
        return gnc::make_error<std::vector<double>>(
            gnc::ErrorCode::NotFound,
            "HDF5 dataset not found: " + ds_path);

    hid_t space = H5Dget_space(ds);
    int   ndims = H5Sget_simple_extent_ndims(space);
    if (ndims != 1) {
        H5Sclose(space); H5Dclose(ds);
        return gnc::make_error<std::vector<double>>(
            gnc::ErrorCode::InvalidArgument,
            ds_path + " is not 1-D");
    }

    hsize_t dims[1] = {0};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    H5Sclose(space);

    std::vector<double> out(dims[0]);
    const herr_t err = H5Dread(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, out.data());
    H5Dclose(ds);
    if (err < 0)
        return gnc::make_error<std::vector<double>>(
            gnc::ErrorCode::IOError,
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

struct EnumContext {
    hid_t                     file_id;
    std::vector<SignalMetadata>* out;
    std::string               sim_id;
};

static herr_t enum_visitor(hid_t /*loc_id*/, const char* name,
                            const H5L_info2_t* /*linfo*/, void* opaque)
{
    auto* ctx = static_cast<EnumContext*>(opaque);

    // Skip the global time axis
    if (std::strcmp(name, "time") == 0) return 0;

    const hid_t ds = H5Dopen2(ctx->file_id, name, H5P_DEFAULT);
    if (ds < 0) return 0;   // not a dataset → skip

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

    // basename = last component
    std::string s(name);
    meta.name = s.substr(s.rfind('/') + 1);

    meta.dtype = hdf5_to_dtype(dtype);
    meta.shape.push_back(static_cast<std::size_t>(dims[0]));
    if (ndims == 2)
        meta.shape.push_back(static_cast<std::size_t>(dims[1]));

    meta.units = read_string_attr(ds, "units");

    H5Tclose(dtype);
    H5Sclose(space);
    H5Dclose(ds);

    ctx->out->push_back(std::move(meta));
    return 0;
}

// ── HDF5Reader implementation ─────────────────────────────────────────────────

HDF5Reader::HDF5Reader() : m_impl(std::make_unique<Impl>()) {}
HDF5Reader::~HDF5Reader() { close(); }

HDF5Reader::HDF5Reader(HDF5Reader&&) noexcept = default;
HDF5Reader& HDF5Reader::operator=(HDF5Reader&&) noexcept = default;

gnc::VoidResult HDF5Reader::open(const std::filesystem::path& path)
{
    if (is_open()) close();

    if (!std::filesystem::exists(path))
        return gnc::make_void_error(gnc::ErrorCode::NotFound,
                                    "File not found: " + path.string());

    // Suppress HDF5 error printing to stderr
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    const hid_t fid = H5Fopen(path.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (fid < 0)
        return gnc::make_void_error(gnc::ErrorCode::IOError,
                                    "H5Fopen failed: " + path.string());

    m_impl->file_id = fid;
    m_impl->path    = path;
    GNC_LOG_INFO("HDF5: opened {}", path.string());
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

gnc::Result<std::vector<SignalMetadata>>
HDF5Reader::enumerate_signals(const std::string& sim_id) const
{
    if (!is_open())
        return gnc::make_error<std::vector<SignalMetadata>>(
            gnc::ErrorCode::InvalidState, "File not open");

    std::vector<SignalMetadata> out;
    EnumContext ctx{m_impl->file_id, &out, sim_id};

    const herr_t err = H5Lvisit2(m_impl->file_id, H5_INDEX_NAME,
                                   H5_ITER_NATIVE, enum_visitor, &ctx);
    if (err < 0)
        return gnc::make_error<std::vector<SignalMetadata>>(
            gnc::ErrorCode::IOError, "H5Lvisit2 failed");

    GNC_LOG_DEBUG("HDF5: enumerated {} signals in {}", out.size(), m_impl->path.string());
    return out;
}

gnc::Result<std::shared_ptr<SignalBuffer>>
HDF5Reader::load_signal(const SignalMetadata& meta) const
{
    if (!is_open())
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            gnc::ErrorCode::InvalidState, "File not open");

    // Load time axis
    const std::string time_path = meta.time_path.empty() ? "/time" : meta.time_path;
    auto time_res = read_1d_doubles(m_impl->file_id, time_path);
    if (!time_res)
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            time_res.error().code,
            "Time axis: " + time_res.error().message);

    // Open signal dataset
    const hid_t ds = H5Dopen2(m_impl->file_id, meta.h5_path.c_str(), H5P_DEFAULT);
    if (ds < 0)
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            gnc::ErrorCode::NotFound,
            "Dataset not found: " + meta.h5_path);

    hid_t space  = H5Dget_space(ds);
    int   ndims  = H5Sget_simple_extent_ndims(space);
    hsize_t dims[2] = {0, 1};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    H5Sclose(space);

    const std::size_t N      = static_cast<std::size_t>(dims[0]);
    const std::size_t n_comp = ndims == 2 ? static_cast<std::size_t>(dims[1]) : 1u;
    std::vector<double> values(N * n_comp);

    const herr_t err = H5Dread(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, values.data());
    H5Dclose(ds);
    if (err < 0)
        return gnc::make_error<std::shared_ptr<SignalBuffer>>(
            gnc::ErrorCode::IOError,
            "H5Dread failed for " + meta.h5_path);

    auto buf = SignalBuffer::make_vector(
        meta, std::move(*time_res), std::move(values), n_comp);
    return buf;
}

} // namespace gnc_viz
