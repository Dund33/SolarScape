#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <utility>
#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/VerletFactory.h"
#include "validation/RecordingValidator.h"

namespace
{
    constexpr Real GRAVITATIONAL_CONSTANT = 0.000000000066743L;
    constexpr Real TIME_STEP = 10.0L;
    constexpr Real SIMULATION_TIME = 63072000.0L;

    constexpr Real PROBE_EMPTY_MASS = 1000.0L;
    constexpr Real PROBE_FUEL_MASS = 1000.0L;
    constexpr Real PROBE_FUEL_FLOW = 1.0L;
    constexpr Real PROBE_SPECIFIC_IMPULSE = 3600.0L;

    auto run() -> int
    {
        VerletFactory simulationFactory(
            GRAVITATIONAL_CONSTANT,
            std::vector<Body>{
                Body(
                    Vector3{0.0L, 0.0L, 0.0L},
                    Vector3{0.0L, 0.0L, 0.0L},
                    1.0e18L)},
            Body(),
            Probe(
                Vector3{1.0e6L, 0.0L, 0.0L},
                Vector3{0.0L, 10.3339L, 0.0L},
                PROBE_EMPTY_MASS,
                PROBE_FUEL_MASS,
                PROBE_FUEL_FLOW,
                PROBE_SPECIFIC_IMPULSE));

        const std::size_t steps =
            static_cast<std::size_t>(
                SIMULATION_TIME / TIME_STEP);

        RecordingValidator validator(
            simulationFactory,
            TIME_STEP,
            steps);

        const std::vector<std::pair<Real, Vector3>> recording =
            validator.record();

        std::ofstream output("period_validation.csv");
        if (!output)
        {
            std::cerr << "Nie mozna utworzyc pliku period_validation.csv\n";
            return 1;
        }

        output << std::setprecision(std::numeric_limits<Real>::max_digits10);
        output << "time,x,y,z\n";
        for (const auto& [time, position] : recording)
        {
            output
                << time << ','
                << position.x << ','
                << position.y << ','
                << position.z << '\n';
        }

        return 0;
    }
}

auto main() -> int
{
    try
    {
        return run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
