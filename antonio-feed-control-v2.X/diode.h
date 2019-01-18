#ifndef _DIODE_DEFINED
#define _DIODE_DEFINED

#ifdef	__cplusplus
extern "C" {
#endif
    
#define NUM_DIODE_ENTRIES 99

#define diode_N 35

struct diode_entry {
    float tempK;
    float voltage;
};


// DT-470
/*
struct diode_entry diode_table[NUM_DIODE_ENTRIES] = {
    {10,   1.420},
    {20,   1.214},
    {30,   1.107},
    {40,   1.088},
    {50,   1.071},
    {60,   1.053},
    {70,   1.034},
    {80,   1.015},
    {90,   0.996},
    {100,  0.976},
    {110,  0.955},
    {120,  0.934},
    {130,  0.912},
    {140,  0.891},
    {150,  0.869},
    {160,  0.847},
    {170,  0.824},
    {180,  0.801},
    {190,  0.779},
    {200,  0.756},
    {210,  0.732},
    {220,  0.709},
    {230,  0.686},
    {240,  0.662},
    {250,  0.638},
    {260,  0.615},
    {270,  0.591},
    {280,  0.567},
    {290,  0.543},
    {300,  0.519},
    {310,  0.495},
    {320,  0.471},
    {330,  0.446},
    {340,  0.422},
    {350,  0.398}
};
*/

struct diode_entry * get_diode_table();

void getdiode_command(char *args[]);
//float auto_start_getdiode();

#ifdef	__cplusplus
}
#endif

#endif