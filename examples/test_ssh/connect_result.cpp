#include <iostream>
#include <thread>
#include <chrono>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

using namespace mavsdk;

int main(int argc, char** argv) {
    // Connect_result(Continue)
    Mavsdk mavsdk;
    ConnectionResult connection_result;
    bool discovered_system = false;
    connection_result = mavsdk.add_any_connection(argv[1]);
    if (connection_result != ConnectionResult::Success) { return 1; } // Failed
        mavsdk.subscribe_on_new_system([&mavsdk, &discovered_system]() {
        const auto system = mavsdk.systems().at(0);
        if (system->is_connected()) { discovered_system = true; } //Discovered system
        }); // Waiting to discover system...
    std::this_thread::sleep_for(std::chrono::seconds(2)); // heartbeats at 1Hz
    if (!discovered_system) { return 1; } // No system found
    const auto system = mavsdk.systems().at(0);
    system->register_component_discovered_callback(
    [](ComponentType component_type) -> void { std::cout << unsigned(component_type) ; }
    ); // Register a callback components (camera, gimbal) etc are found

    // Regist telemetry(Continue)
    auto telemetry = std::make_shared<Telemetry>(system);// We want to listen to the altitude of the drone at 1 Hz.
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);
    if (set_rate_result != Telemetry::Result::Success) { return 1; } // Set rate failed
    telemetry->subscribe_position([](Telemetry::Position position) {
    std::cout << "Altitude: " << position.relative_altitude_m << " m" << std::endl;
    }); // Set up callback to monitor altitude
    while (telemetry->health_all_ok() != true) {
    std::cout << "Vehicle is getting ready to arm" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    } // Check if vehicle is ready to arm

    // Arm(Continue)
    auto action = std::make_shared<Action>(system);
    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action->arm();

    if (arm_result != Action::Result::Success) {
    std::cout << "Arming failed:" << arm_result << std::endl;
    return 1;
    }

    // Takeoff(Continue)
    std::cout << "Taking off..." << std::endl;
    const Action::Result takeoff_result = action->takeoff();
    if (takeoff_result != Action::Result::Success) {
    std::cout << "Takeoff failed:" << takeoff_result << std::endl;
    return 1;
    }

    // land (continue)
    std::this_thread::sleep_for(std::chrono::seconds(10)); // stuck air
    std::cout << "Landing..." << std::endl;
    const Action::Result land_result = action->land();
    if (land_result != Action::Result::Success) { return 1; } // Land failed
    while (telemetry->in_air()) { // Check if vehicle is still in air
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Vehicle is landing...
    }
    std::cout << "Landed!" << std::endl;
    // Relying on auto-disarming but let's keep watching the telemetry for a bit longer.
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "Finished..." << std::endl;
    return 0;
}
