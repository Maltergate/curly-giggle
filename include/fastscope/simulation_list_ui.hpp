#pragma once
/// @file simulation_list_ui.hpp
/// @brief Renders the simulation files list in the left pane.
/// @ingroup ui_widgets

#include <filesystem>

namespace fastscope {
struct AppState;

/// @brief Open a single HDF5 file and append it to state.simulations.
/// @param state   Application state that receives the new simulation.
/// @param p       Path to the .h5 / .hdf5 file.
/// @param sim_id  Unique identifier string for this simulation.
void open_simulation_file(AppState& state,
                          const std::filesystem::path& p,
                          const std::string& sim_id);

/// @brief Render the "Simulations" left pane content.
/// @param state Application state (mutable so files can be loaded/unloaded).
void render_simulation_list(AppState& state);

} // namespace fastscope
