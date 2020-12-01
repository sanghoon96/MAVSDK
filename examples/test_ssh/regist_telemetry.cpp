#include <iostream> 
#include <thread> 
#include <chrono>
#include <mavsdk/mavsdk.h> 
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

using namespace mavsdk;

int main(int argc, char** argv) {
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
return 0;
}