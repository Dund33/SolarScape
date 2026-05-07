//
// Created by Luke on 5/7/2026.
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "Body.h"
#include "computation/Verlet.h"

int main()
{
    const double gravitationalConstant = 6.67430e-11;
    const double timeStep = 60.0 * 60.0;
    const int steps = 24 * 365;

    std::vector<Body> bodies;
    bodies.push_back(Body(Vector3(0.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0), 1.98847e30));
    bodies.push_back(Body(Vector3(149597870700.0, 0.0, 0.0), Vector3(0.0, 29780.0, 0.0), 5.9722e24));

    std::ofstream output("simulation.csv");
    if (!output)
    {
        std::cerr << "Nie mozna utworzyc pliku simulation.csv\n";
        return 1;
    }

    output << std::setprecision(17);
    output << "step,time,body,x,y,z,vx,vy,vz,mass\n";

    for (int step = 0; step <= steps; ++step)
    {
        const double time = step * timeStep;

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            output << step << ','
                   << time << ','
                   << i << ','
                   << bodies[i].position.x << ','
                   << bodies[i].position.y << ','
                   << bodies[i].position.z << ','
                   << bodies[i].velocity.x << ','
                   << bodies[i].velocity.y << ','
                   << bodies[i].velocity.z << ','
                   << bodies[i].mass << '\n';
        }

        if (step < steps)
        {
            Verlet::step(bodies, timeStep, gravitationalConstant);
        }
    }

    return 0;
}
