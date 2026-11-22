#include <DualSketchDemo.h>

static DualSketchDemo demo;

void setup()
{
    demo.begin("Dual Sketch: Counter", -165);
}

void loop()
{
    demo.tick(40);
}

// Runs on CPU 0, including when loop() is assigned to a worker CPU.
extern "C" void xdos_sketch_service()
{
    demo.service();
}
