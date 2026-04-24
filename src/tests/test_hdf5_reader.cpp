// test_hdf5_reader.cpp — unit tests for HDF5Reader
//
// Creates synthetic in-memory HDF5 files using H5P_FAPL_CORE to avoid
// writing to disk. Tests enumerate_signals and load_signal.

#include "fastscope/hdf5_reader.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <hdf5.h>

#include <cstring>
#include <vector>

// ── Helpers to create in-memory HDF5 files ────────────────────────────────────

/// RAII wrapper for an in-memory HDF5 file that stores to a temp path.
struct InMemH5 {
    hid_t file_id = H5I_INVALID_HID;
    std::string tmp_path;

    static InMemH5 create(const char* name)
    {
        InMemH5 h;
        h.tmp_path = std::string("/tmp/gnc_test_") + name + ".h5";

        hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
        // 1 MB increment; backing_store=0 → pure in-memory
        H5Pset_fapl_core(fapl, 1024 * 1024, 0);

        // Suppress HDF5 error printing
        H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

        h.file_id = H5Fcreate(h.tmp_path.c_str(),
                              H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
        H5Pclose(fapl);
        return h;
    }

    ~InMemH5() { if (file_id != H5I_INVALID_HID) H5Fclose(file_id); }

    /// Write a 1-D double dataset under `name`.
    void write1D(const char* name, const std::vector<double>& data)
    {
        hsize_t dims[1] = { data.size() };
        hid_t space = H5Screate_simple(1, dims, nullptr);
        hid_t ds    = H5Dcreate2(file_id, name, H5T_NATIVE_DOUBLE,
                                  space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Dwrite(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
        H5Dclose(ds);
        H5Sclose(space);
    }

    /// Write a 2-D double dataset {rows × cols}.
    void write2D(const char* name, const std::vector<double>& data,
                 hsize_t rows, hsize_t cols)
    {
        hsize_t dims[2] = { rows, cols };
        hid_t space = H5Screate_simple(2, dims, nullptr);
        hid_t ds    = H5Dcreate2(file_id, name, H5T_NATIVE_DOUBLE,
                                  space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Dwrite(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
        H5Dclose(ds);
        H5Sclose(space);
    }

    /// Write a string attribute on a dataset.
    void setAttr(const char* ds_name, const char* attr_name, const char* value)
    {
        hid_t ds     = H5Dopen2(file_id, ds_name, H5P_DEFAULT);
        hid_t atype  = H5Tcopy(H5T_C_S1);
        H5Tset_size(atype, std::strlen(value) + 1);
        hid_t aspace = H5Screate(H5S_SCALAR);
        hid_t attr   = H5Acreate2(ds, attr_name, atype, aspace,
                                   H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, atype, value);
        H5Aclose(attr);
        H5Sclose(aspace);
        H5Tclose(atype);
        H5Dclose(ds);
    }

    /// Save to a real temp file and return the path (needed so HDF5Reader can open it).
    std::string flush_to_disk()
    {
        // Re-create with backing_store=1
        hid_t fapl2 = H5Pcreate(H5P_FILE_ACCESS);
        H5Pset_fapl_core(fapl2, 1024 * 1024, 1);  // backing_store=1 → write on close

        H5Fclose(file_id);
        file_id = H5Fopen(tmp_path.c_str(), H5F_ACC_RDONLY, fapl2);
        H5Pclose(fapl2);
        return tmp_path;
    }
};

/// Build a simple in-memory HDF5 file and save to disk.
static std::string make_test_h5(const char* tag)
{
    // Write with backing_store=1 from the start so HDF5Reader can open it.
    std::string path = std::string("/tmp/gnc_test_") + tag + ".h5";

    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_core(fapl, 1024 * 1024, 1);
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    hid_t fid = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
    H5Pclose(fapl);

    // time vector: 0, 0.1, 0.2, …, 0.9
    std::vector<double> time = {0.0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9};

    // scalar signal: altitude = [100,101,102,…,109]
    std::vector<double> alt(10);
    for (int i=0;i<10;++i) alt[i] = 100.0 + i;

    // vector signal: velocity 10×3
    std::vector<double> vel(30);
    for (int i=0;i<10;++i) { vel[i*3]=1.0*i; vel[i*3+1]=2.0*i; vel[i*3+2]=3.0*i; }

    auto write1D = [&](const char* name, const std::vector<double>& d) {
        hsize_t dims[1] = {d.size()};
        hid_t sp = H5Screate_simple(1, dims, nullptr);
        hid_t ds = H5Dcreate2(fid, name, H5T_NATIVE_DOUBLE, sp, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Dwrite(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, d.data());
        H5Dclose(ds); H5Sclose(sp);
    };
    write1D("time", time);
    write1D("altitude", alt);

    // set units attribute on altitude
    {
        hid_t ds    = H5Dopen2(fid, "altitude", H5P_DEFAULT);
        const char* u = "m";
        hid_t at    = H5Tcopy(H5T_C_S1);
        H5Tset_size(at, 2);
        hid_t asp   = H5Screate(H5S_SCALAR);
        hid_t attr  = H5Acreate2(ds, "units", at, asp, H5P_DEFAULT, H5P_DEFAULT);
        H5Awrite(attr, at, u);
        H5Aclose(attr); H5Sclose(asp); H5Tclose(at); H5Dclose(ds);
    }

    // 2-D velocity
    {
        hsize_t dims2[2] = {10, 3};
        hid_t sp = H5Screate_simple(2, dims2, nullptr);
        hid_t ds = H5Dcreate2(fid, "velocity", H5T_NATIVE_DOUBLE, sp, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        H5Dwrite(ds, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, vel.data());
        H5Dclose(ds); H5Sclose(sp);
    }

    H5Fclose(fid);
    return path;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE("HDF5Reader: open non-existent file returns error") {
    fastscope::HDF5Reader r;
    auto res = r.open("/tmp/does_not_exist_gnc.h5");
    REQUIRE(!res);
    REQUIRE(!r.is_open());
}

TEST_CASE("HDF5Reader: open + close") {
    std::string path = make_test_h5("open_close");
    fastscope::HDF5Reader r;
    auto res = r.open(path);
    REQUIRE(res);
    REQUIRE(r.is_open());
    r.close();
    REQUIRE(!r.is_open());
}

TEST_CASE("HDF5Reader: enumerate_signals includes all datasets (including time)") {
    std::string path = make_test_h5("enumerate");
    fastscope::HDF5Reader r;
    REQUIRE(r.open(path));
    auto res = r.enumerate_signals("sim1");
    REQUIRE(res);
    const auto& sigs = *res;
    REQUIRE(sigs.size() == 3);  // time + altitude + velocity

    bool found_time = false, found_alt = false, found_vel = false;
    for (const auto& m : sigs) {
        if (m.name == "time")     { found_time = true; }
        if (m.name == "altitude") {
            found_alt = true;
            CHECK(m.sim_id == "sim1");
            CHECK(m.units == "m");
            CHECK(m.dtype == fastscope::DataType::Float64);
            CHECK(m.shape.size() == 1);
            CHECK(m.shape[0] == 10);
        }
        if (m.name == "velocity") {
            found_vel = true;
            CHECK(m.shape.size() == 2);
            CHECK(m.shape[0] == 10);
            CHECK(m.shape[1] == 3);
        }
    }
    REQUIRE(found_time);
    REQUIRE(found_alt);
    REQUIRE(found_vel);
}

TEST_CASE("HDF5Reader: suggest_time_axes finds time dataset") {
    std::string path = make_test_h5("suggest_time");
    fastscope::HDF5Reader r;
    REQUIRE(r.open(path));
    auto candidates = r.suggest_time_axes();
    REQUIRE(!candidates.empty());
    CHECK(candidates[0] == "/time");
}

TEST_CASE("HDF5Reader: load scalar signal") {
    using Catch::Matchers::WithinAbs;
    std::string path = make_test_h5("load_scalar");
    fastscope::HDF5Reader r;
    REQUIRE(r.open(path));

    auto sigs = r.enumerate_signals("simA");
    REQUIRE(sigs);

    const fastscope::SignalMetadata* alt_meta = nullptr;
    for (const auto& m : *sigs)
        if (m.name == "altitude") { alt_meta = &m; break; }
    REQUIRE(alt_meta != nullptr);

    // Set the time axis explicitly (user would select this in the UI)
    fastscope::SignalMetadata meta_with_time = *alt_meta;
    meta_with_time.time_path = "/time";

    auto buf_res = r.load_signal(meta_with_time);
    REQUIRE(buf_res);
    auto buf = *buf_res;
    REQUIRE(buf != nullptr);
    CHECK(buf->sample_count() == 10);
    CHECK(buf->n_components() == 1);

    REQUIRE_THAT(buf->at(0), WithinAbs(100.0, 1e-9));
    REQUIRE_THAT(buf->at(9), WithinAbs(109.0, 1e-9));
    REQUIRE_THAT(buf->time()[0], WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(buf->time()[9], WithinAbs(0.9, 1e-9));
}

TEST_CASE("HDF5Reader: load vector signal") {
    using Catch::Matchers::WithinAbs;
    std::string path = make_test_h5("load_vector");
    fastscope::HDF5Reader r;
    REQUIRE(r.open(path));

    auto sigs = r.enumerate_signals("simB");
    REQUIRE(sigs);

    const fastscope::SignalMetadata* vel_meta = nullptr;
    for (const auto& m : *sigs)
        if (m.name == "velocity") { vel_meta = &m; break; }
    REQUIRE(vel_meta != nullptr);

    fastscope::SignalMetadata meta_with_time = *vel_meta;
    meta_with_time.time_path = "/time";

    auto buf_res = r.load_signal(meta_with_time);
    REQUIRE(buf_res);
    auto buf = *buf_res;
    REQUIRE(buf != nullptr);
    CHECK(buf->sample_count() == 10);
    CHECK(buf->n_components() == 3);

    // velocity[5] = {5.0, 10.0, 15.0}
    REQUIRE_THAT(buf->at(5, 0), WithinAbs(5.0,  1e-9));
    REQUIRE_THAT(buf->at(5, 1), WithinAbs(10.0, 1e-9));
    REQUIRE_THAT(buf->at(5, 2), WithinAbs(15.0, 1e-9));
}

TEST_CASE("HDF5Reader: enumerate_signals auto-discovers time_path") {
    using Catch::Matchers::WithinAbs;
    std::string path = make_test_h5("auto_time");
    fastscope::HDF5Reader r;
    REQUIRE(r.open(path));

    auto sigs = r.enumerate_signals();
    REQUIRE(sigs);

    const fastscope::SignalMetadata* alt_meta = nullptr;
    for (const auto& m : *sigs)
        if (m.name == "altitude") { alt_meta = &m; break; }
    REQUIRE(alt_meta != nullptr);

    // enumerate_signals() should have resolved time_path to "/time"
    CHECK(alt_meta->time_path == "/time");

    // load_signal uses the auto-discovered time axis (0.0, 0.1, …, 0.9)
    auto buf_res = r.load_signal(*alt_meta);
    REQUIRE(buf_res);
    auto buf = *buf_res;
    REQUIRE_THAT(buf->time()[0], WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(buf->time()[5], WithinAbs(0.5, 1e-9));
    REQUIRE_THAT(buf->time()[9], WithinAbs(0.9, 1e-9));
}

TEST_CASE("HDF5Reader: empty time_path returns error") {
    fastscope::HDF5Reader r;
    std::string path = make_test_h5("no_time_err");
    REQUIRE(r.open(path));

    fastscope::SignalMetadata meta;
    meta.h5_path  = "/altitude";
    meta.time_path = "";   // explicitly empty — no auto-discovery
    auto res = r.load_signal(meta);
    REQUIRE(!res);
    CHECK(res.error().code == fastscope::ErrorCode::NotFound);
}

TEST_CASE("HDF5Reader: load_signal without open returns error") {
    fastscope::HDF5Reader r;
    fastscope::SignalMetadata meta;
    meta.h5_path = "/altitude";
    auto res = r.load_signal(meta);
    REQUIRE(!res);
}

TEST_CASE("HDF5Reader: enumerate without open returns error") {
    fastscope::HDF5Reader r;
    auto res = r.enumerate_signals();
    REQUIRE(!res);
}
