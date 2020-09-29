/*
* omega.cl
*
*  date    : 19.08.2020
*  author  : M. Khaled
*/

/* some defines needed later */
#define concrete_t @@concrete_t_name@@
#define symbolic_t @@symbolic_t_name@@
#define FLAT_TYPE symbolic_t
#define FLAT_TYPE_SIZE 1
#define flat_t FLAT_TYPE

// load some values from the config file
#define ssDim @pfaces-configValue:"system.states.dimension"
#define ssQnt @pfaces-configValue:"system.states.quantizers"
#define ssLb @pfaces-configValue:"system.states.first_symbol"
#define ssUb @pfaces-configValue:"system.states.last_symbol"
#define isDim @pfaces-configValue:"system.controls.dimension"
#define isQnt @pfaces-configValue:"system.controls.quantizers"
#define isLb @pfaces-configValue:"system.controls.first_symbol"
#define isUb @pfaces-configValue:"system.controls.last_symbol"

/* pfaces things */
#include "pfaces.cl"

/* utility functions */
@pfaces-include:"omega_utils.cl"

/* kernel functions for identifying the artomic propositions */
@pfaces-include:"omega_aps.cl"

/* a kernel function for constructing the symbolic model */
@pfaces-include:"omega_symmodel.cl"