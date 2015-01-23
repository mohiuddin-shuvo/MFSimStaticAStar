/*------------------------------------------------------------------------------*
 *                       (c)2014, All Rights Reserved.     						*
 *       ___           ___           ___     									*
 *      /__/\         /  /\         /  /\    									*
 *      \  \:\       /  /:/        /  /::\   									*
 *       \  \:\     /  /:/        /  /:/\:\  									*
 *   ___  \  \:\   /  /:/  ___   /  /:/~/:/        								*
 *  /__/\  \__\:\ /__/:/  /  /\ /__/:/ /:/___     UCR DMFB Synthesis Framework  *
 *  \  \:\ /  /:/ \  \:\ /  /:/ \  \:\/:::::/     www.microfluidics.cs.ucr.edu	*
 *   \  \:\  /:/   \  \:\  /:/   \  \::/~~~~ 									*
 *    \  \:\/:/     \  \:\/:/     \  \:\     									*
 *     \  \::/       \  \::/       \  \:\    									*
 *      \__\/         \__\/         \__\/    									*
 *-----------------------------------------------------------------------------*/
/*--------------------------------File Details----------------------------------*
 * Name: Enums (Enumerations)													*
 *																				*
 * Details: This file contains a number of enumerated types used to make the	*
 * code easier to read and understand.											*
 *-----------------------------------------------------------------------------*/
#ifndef _ENUMS_H
#define _ENUMS_H

enum Direction { EAST, WEST, NORTH, SOUTH, ON_TARGET, DIR_UNINIT };
enum AssayNodeStatus { UNBOUND_UNSCHED = 0, SCHEDULED, BOUND, ROUTED, EXECUTING, COMPLETE };
enum ResourceType { BASIC_RES = 0, D_RES = 1, H_RES = 2, DH_RES = 3, SSD_RES = 4, RB_RES = 5, RES_TYPE_MAX = RB_RES, UNKNOWN_RES }; // Warning: Changing this order requires a change in the visualizer's parser
enum OperationType { GENERAL, STORAGE_HOLDER, STORAGE, DISPENSE, MIX, SPLIT, DILUTE, HEAT, DETECT, OUTPUT, WASH };
enum DropletStatus { DROP_NORMAL, DROP_WAIT, DROP_INT_WAIT, DROP_PROCESSING, DROP_OUTPUT, DROP_MERGING, DROP_SPLITTING, DROP_DRIFT, DROP_WASH, DROP_WASTE };
enum ExecutionType { SIM_EX, PROG_EX, ALL_EX }; // Is program being run to generate simulation or actual DMFB program
enum WireSegType { LINE_WS, ARC_WS };
enum KamerNodeType { OE, IE, GE, RW };
enum ModuleDeltaType { TS_MDT, C_MDT }; // Warning: Changing this order requires a change in the visualizer's parser

// Synthesis types
enum SchedulerType { LIST_S, PATH_S, GENET_S, RICKETT_S, FD_LIST_S, FPPC_S, FPPC_PATH_S, RT_EVAL_LIST_S, SKYCAL_S /*,NEW_S*/};
enum PlacerType { GRISSOM_LE_B, GRISSOM_PATH_B, SA_P, KAMER_LL_P, FPPC_LE_B, SKYCAL_P /*,NEW_P*/}; // Placer OR Binder
enum RouterType { GRISSOM_FIX_R, GRISSOM_FIX_MAP_R, ROY_MAZE_R, BIOROUTE_R, FPPC_SEQUENTIAL_R, FPPC_PARALLEL_R, CHO_R, LEE_R, SKYCAL_R ,
	A_STAR_TYPE_1_R, A_STAR_TYPE_2_R, A_STAR_TYPE_3_R,
	PR_A_STAR_TYPE_1S_R, PR_A_STAR_TYPE_2S_R, PR_A_STAR_TYPE_3S_R,
	PR_A_STAR_TYPE_1L_R, PR_A_STAR_TYPE_2L_R, PR_A_STAR_TYPE_3L_R,
	PR_A_STAR_TYPE_1R_R, PR_A_STAR_TYPE_2R_R, PR_A_STAR_TYPE_3R_R,
	PR_A_STAR_TYPE_1X_R, PR_A_STAR_TYPE_2X_R, PR_A_STAR_TYPE_3X_R,
	PR_A_STAR_TYPE_1Y_R, PR_A_STAR_TYPE_2Y_R, PR_A_STAR_TYPE_3Y_R,
	PR_A_STAR_TYPE_1B_R, PR_A_STAR_TYPE_2B_R, PR_A_STAR_TYPE_3B_R /*,NEW_R*/};

// Scheduling utility types
enum ResourceAllocationType { GRISSOM_FIX_0_RA, GRISSOM_FIX_1_RA, GRISSOM_FIX_2_RA, GRISSOM_FIX_3_RA, PC_INHERENT_RA, INHERIT_RA }; // Determines how many resources (and what type) will be available for the scheduler

// Routing utility types
enum CompactionType { NO_COMP, BEG_COMP, MID_COMP, CHO_COMP, DYN_COMP, INHERENT_COMP,
	A_STAR_COMP, PR_A_STAR_TYPE_S_COMP, PR_A_STAR_TYPE_L_COMP, PR_A_STAR_TYPE_R_COMP, PR_A_STAR_TYPE_X_COMP, PR_A_STAR_TYPE_Y_COMP, PR_A_STAR_TYPE_B_COMP };
enum ProcessEngineType { FIXED_PE, FREE_PE, FPPC_PE }; // How to move droplets in modules

// Pin-mapping types
enum PinMapType { INDIVADDR_PM, CLIQUE_PM, ORIGINAL_FPPC_PM, ENHANCED_FPPC_PIN_OPT_PM, ENHANCED_FPPC_ROUTE_OPT_PM };

// Wire-routing types
enum WireRouteType { NONE_WR, PATH_FINDER_WR, YEH_WR, ENHANCED_FPPC_WR };
enum WireRouteNodeType { PIN_WRN, EMPTY_PIN_WRN, ESCAPE_WRN, INTERNAL_WRN, SUPER_ESCAPE_WRN };

// Routing status enums
enum SoukupC { NotReached = 0, LeeReached = 1, LineReached = 2 };
enum SoukupS { OtherLayer = 0, LeftT = 1, RightT = 2, DownT = 3, UpT = 4, StartPoint = 5, TargetPoint = 6, Blockage = 7, UnknownS = 100};

// Pin-mapping tests
enum PinConstrainedPinMapTestGeneratorType { JETC_2D_MESH_TPM, JETC2_2D_MESH_TPM };

#endif
