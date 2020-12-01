#include <chrono>
#include <cstdint>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/mission/mission.h>
#include <iostream>
#include <thread>
#include <future>

using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;

int main(int argc, char** argv){

    // 01. Connect_result
    Mavsdk mavsdk;
    ConnectionResult connection_result;
    bool discovered_system = false;
    connection_result = mavsdk.add_any_connection(argv[1]);

    if(connection_result != ConnectionResult::Success){return 1;}
    
    mavsdk.subscribe_on_new_system([&mavsdk, &discovered_system](){
        const auto system = mavsdk.systems().at(0);
        if(system->is_connected()){discovered_system=true;}
    });
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if(!discovered_system){return 1;}
    
    const auto system = mavsdk.systems().at(0);
    system->register_component_discovered_callback(
        [](ComponentType component_type)->void {std::cout<<unsigned(component_type);}
    );

    // 02. Register Telemetry
    auto telemetry = std::make_shared<Telemetry>(system);
    const Telemetry::Result set_rate_result = telemetry->set_rate_position(1.0);
    if(set_rate_result != Telemetry::Result::Success){return 1;}
    telemetry->subscribe_position([](Telemetry::Position position){
        std::cout<<"Altitude:"<<position.relative_altitude_m<<"m"<<std::endl;
    });
    while(telemetry->health_all_ok()!=true){
        std::cout<<"Vehicle is getting ready to arm"<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 03. Arm
    auto action = std::make_shared<Action>(system);
    std::cout<<"Arming..."<<std::endl;
    const Action::Result arm_result=action->arm();

    if(arm_result != Action::Result::Success){
        std::cout<<"Arming failed."<<arm_result<<std::endl;
        return 1;
    }

        // 06-1. Upload Mission(1) 
    std::vector<Mission::MissionItem> mission_items;
    Mission::MissionItem mission_item;
    mission_item.latitude_deg = 47.398170327054473; // range: -90 to +90
    mission_item.longitude_deg = 8.5456490218639658; // range: -180 to +180
    mission_item.relative_altitude_m = 10.0f; // takeoff altitude
    mission_item.speed_m_s = 50.0f;
    mission_item.is_fly_through = false;
    mission_items.push_back(mission_item);

    // 47.397960, 8.546035
    mission_item.latitude_deg = 47.397960; // range: -90 to +90
    mission_item.longitude_deg = 8.546035; // range: -180 to +180
    mission_item.relative_altitude_m = 10.0f; // takeoff altitude
    mission_item.speed_m_s = 50.0f;
    mission_item.is_fly_through = false;
    mission_items.push_back(mission_item);

    int mission_size = mission_items.size();
    std::cout << mission_size << std::endl;
    
    // 6-2. Upload Mission(2)
    auto mission = std::make_shared<Mission>(system);
    {
        auto prom = std::make_shared<std::promise<Mission::Result>>();
        auto future_result = prom->get_future();
        Mission::MissionPlan mission_plan;
        mission_plan.mission_items = mission_items;
        mission->upload_mission_async(mission_plan,[prom](Mission::Result result){prom->set_value(result);});
        const Mission::Result result = future_result.get();
        if (result != Mission::Result::Success) { return 1; }
    }

    // 07. Mission Progress (04. Takeoff 지우고 실행해야 함)
    {
        std::cout << "Starting mission." << std::endl;
        auto start_prom = std::make_shared<std::promise<Mission::Result>>();
        auto future_result = start_prom->get_future();
        mission->start_mission_async([start_prom](Mission::Result result) {
        start_prom->set_value(result);
        std::cout << "Started mission." << std::endl;
        });
        const Mission::Result result = future_result.get();
        if (result != Mission::Result::Success) { return -1; } // Mission start failed
        while (!mission->is_mission_finished().second) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Not finished mission.
        }
    }

    /*
    // 04. Takeoff
    std::cout << "Taking off..." << std::endl;
    const Action::Result takeoff_result = action->takeoff();
    if(takeoff_result != Action::Result::Success){
        std::cout << "Takeoff failed." << takeoff_result << std::endl;
        return 1; 
    }
    */

    // 05. Land
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout<<"Landing..."<<std::endl;
        const Action::Result land_result = action->land();
        if(land_result != Action::Result::Success){return 1;}
        while(telemetry->in_air()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout<<"Landed!"<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout<<"Finished..."<<std::endl;
    }
    return 0;
}