// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2013-2017 NVIDIA Corporation. All rights reserved.

#ifndef NV_FLEX_H
#define NV_FLEX_H

//! \cond HIDDEN_SYMBOLS
#if _WIN32
#define NV_FLEX_API __declspec(dllexport)
#else
#define NV_FLEX_API
#endif

// least 2 significant digits define minor version, eg: 10 -> version 0.10
#define NV_FLEX_VERSION 110

//! \endcond

/** \file NvFlex.h
 * The main include file for the core Flex solver.
 */

extern "C" {

/**
 * Opaque type representing a library that can create FlexSolvers, FlexTriangleMeshes, and NvFlexBuffers
 */
typedef struct NvFlexLibrary NvFlexLibrary;

/**
 * Opaque type representing a collection of particles and constraints
 */
typedef struct NvFlexSolver NvFlexSolver;

/**
 * Opaque type representing a data buffer, type and contents depends on usage, see NvFlexAllocBuffer()
 */
typedef struct NvFlexBuffer NvFlexBuffer;

/**
 * Controls behavior of NvFlexMap()
 */
enum NvFlexMapFlags
{
   	eNvFlexMapWait		= 0,	//!< Calling thread will be blocked until buffer is ready for access, default
   	eNvFlexMapDoNotWait = 1,	//!< Calling thread will check if buffer is ready for access, if not ready then the method will return NULL immediately
	eNvFlexMapDiscard	= 2		//!< Buffer contents will be discarded, this allows for efficent buffer reuse
};

/**
 * Controls memory space of a NvFlexBuffer, see NvFlexAllocBuffer()
 */
enum NvFlexBufferType
{
   	eNvFlexBufferHost	= 0,	//!< Host mappable buffer, pinned memory on CUDA, staging buffer on DX
   	eNvFlexBufferDevice	= 1,	//!< Device memory buffer, mapping this on CUDA will return a device memory pointer, and will return a buffer pointer on DX
};

/**
 * Controls the relaxation method used by the solver to ensure convergence
 */
enum NvFlexRelaxationMode
{
	eNvFlexRelaxationGlobal = 0,	//!< The relaxation factor is a fixed multiplier on each constraint's position delta
	eNvFlexRelaxationLocal  = 1	//!< The relaxation factor is a fixed multiplier on each constraint's delta divided by the particle's constraint count, convergence will be slower but more reliable
};


/**
 * Simulation parameters for a solver
 */
struct NvFlexParams
{
	int numIterations;					//!< Number of solver iterations to perform per-substep

	float gravity[3];					//!< Constant acceleration applied to all particles
	float radius;						//!< The maximum interaction radius for particles
	float solidRestDistance;			//!< The distance non-fluid particles attempt to maintain from each other, must be in the range (0, radius]
	float fluidRestDistance;			//!< The distance fluid particles are spaced at the rest density, must be in the range (0, radius], for fluids this should generally be 50-70% of mRadius, for rigids this can simply be the same as the particle radius

	// common params
	float dynamicFriction;				//!< Coefficient of friction used when colliding against shapes
	float staticFriction;				//!< Coefficient of static friction used when colliding against shapes
	float particleFriction;				//!< Coefficient of friction used when colliding particles
	float restitution;					//!< Coefficient of restitution used when colliding against shapes, particle collisions are always inelastic
	float adhesion;						//!< Controls how strongly particles stick to surfaces they hit, default 0.0, range [0.0, +inf]
	float sleepThreshold;				//!< Particles with a velocity magnitude < this threshold will be considered fixed
	
	float maxSpeed;						//!< The magnitude of particle velocity will be clamped to this value at the end of each step
	float maxAcceleration;				//!< The magnitude of particle acceleration will be clamped to this value at the end of each step (limits max velocity change per-second), useful to avoid popping due to large interpenetrations
	
	float shockPropagation;				//!< Artificially decrease the mass of particles based on height from a fixed reference point, this makes stacks and piles converge faster
	float dissipation;					//!< Damps particle velocity based on how many particle contacts it has
	float damping;						//!< Viscous drag force, applies a force proportional, and opposite to the particle velocity

	// cloth params
	float wind[3];						//!< Constant acceleration applied to particles that belong to dynamic triangles, drag needs to be > 0 for wind to affect triangles
	float drag;							//!< Drag force applied to particles belonging to dynamic triangles, proportional to velocity^2*area in the negative velocity direction
	float lift;							//!< Lift force applied to particles belonging to dynamic triangles, proportional to velocity^2*area in the direction perpendicular to velocity and (if possible), parallel to the plane normal

	// fluid params
	bool fluid;							//!< If true then particles with phase 0 are considered fluid particles and interact using the position based fluids method
	float cohesion;						//!< Control how strongly particles hold each other together, default: 0.025, range [0.0, +inf]
	float surfaceTension;				//!< Controls how strongly particles attempt to minimize surface area, default: 0.0, range: [0.0, +inf]    
	float viscosity;					//!< Smoothes particle velocities using XSPH viscosity
	float vorticityConfinement;			//!< Increases vorticity by applying rotational forces to particles
	float anisotropyScale;				//!< Control how much anisotropy is present in resulting ellipsoids for rendering, if zero then anisotropy will not be calculated, see NvFlexGetAnisotropy()
	float anisotropyMin;				//!< Clamp the anisotropy scale to this fraction of the radius
	float anisotropyMax;				//!< Clamp the anisotropy scale to this fraction of the radius
	float smoothing;					//!< Control the strength of Laplacian smoothing in particles for rendering, if zero then smoothed positions will not be calculated, see NvFlexGetSmoothParticles()
	float solidPressure;				//!< Add pressure from solid surfaces to particles
	float freeSurfaceDrag;				//!< Drag force applied to boundary fluid particles
	float buoyancy;						//!< Gravity is scaled by this value for fluid particles

	// diffuse params
	float diffuseThreshold;				//!< Particles with kinetic energy + divergence above this threshold will spawn new diffuse particles
	float diffuseBuoyancy;				//!< Scales force opposing gravity that diffuse particles receive
	float diffuseDrag;					//!< Scales force diffuse particles receive in direction of neighbor fluid particles
	int diffuseBallistic;				//!< The number of neighbors below which a diffuse particle is considered ballistic
	float diffuseSortAxis[3];			//!< Diffuse particles will be sorted by depth along this axis if non-zero
	float diffuseLifetime;				//!< Time in seconds that a diffuse particle will live for after being spawned, particles will be spawned with a random lifetime in the range [0, diffuseLifetime]

	// rigid params
	float plasticThreshold;				//!< Particles belonging to rigid shapes that move with a position delta magnitude > threshold will be permanently deformed in the rest pose
	float plasticCreep;					//!< Controls the rate at which particles in the rest pose are deformed for particles passing the deformation threshold 

	// collision params
	float collisionDistance;			//!< Distance particles maintain against shapes, note that for robust collision against triangle meshes this distance should be greater than zero
	float particleCollisionMargin;		//!< Increases the radius used during neighbor finding, this is useful if particles are expected to move significantly during a single step to ensure contacts aren't missed on subsequent iterations
	float shapeCollisionMargin;			//!< Increases the radius used during contact finding against kinematic shapes

	float planes[8][4];					//!< Collision planes in the form ax + by + cz + d = 0
	int numPlanes;						//!< Num collision planes

	NvFlexRelaxationMode relaxationMode;//!< How the relaxation is applied inside the solver
	float relaxationFactor;				//!< Control the convergence rate of the parallel solver, default: 1, values greater than 1 may lead to instability
};

/**
 * Flags that control the a particle's behavior and grouping, use NvFlexMakePhase() to construct a valid 32bit phase identifier
 */
enum NvFlexPhase
{
	eNvFlexPhaseGroupMask			= 0x00ffffff,	//!< Low 24 bits represent the particle group for controlling collisions	

	eNvFlexPhaseSelfCollide			= 1 << 24,		//!< If set this particle will interact with particles of the same group
	eNvFlexPhaseSelfCollideFilter	= 1 << 25,		//!< If set this particle will ignore collisions with particles closer than the radius in the rest pose, this flag should not be specified unless valid rest positions have been specified using NvFlexSetRestParticles()
	eNvFlexPhaseFluid				= 1 << 26,		//!< If set this particle will generate fluid density constraints for its overlapping neighbors
};

/**
 * Generate a bit set for the particle phase, the group should be an integer < 2^24, and the flags should be a combination of FlexPhase enum values
 */
NV_FLEX_API inline int NvFlexMakePhase(int group, int flags) { return (group & eNvFlexPhaseGroupMask) | flags; }


/**
 * Time spent in each section of the solver update, times in GPU seconds, see NvFlexUpdateSolver()
 */
struct NvFlexTimers
{
	float predict;				//!< Time spent in prediction
	float createCellIndices;	//!< Time spent creating grid indices
	float sortCellIndices;		//!< Time spent sorting grid indices
	float createGrid;			//!< Time spent creating grid
	float reorder;				//!< Time spent reordering particles
	float collideParticles;		//!< Time spent finding particle neighbors
	float collideShapes;		//!< Time spent colliding convex shapes
	float collideTriangles;		//!< Time spent colliding triangle shapes
	float collideFields;		//!< Time spent colliding signed distance field shapes
	float calculateDensity;		//!< Time spent calculating fluid density
	float solveDensities;		//!< Time spent solving density constraints
	float solveVelocities;		//!< Time spent solving velocity constraints
	float solveShapes;			//!< Time spent solving rigid body constraints
	float solveSprings;			//!< Time spent solving distance constraints
	float solveContacts;		//!< Time spent solving contact constraints
	float solveInflatables;		//!< Time spent solving pressure constraints
	float applyDeltas;	        //!< Time spent adding position deltas to particles
	float calculateAnisotropy;	//!< Time spent calculating particle anisotropy for fluid
	float updateDiffuse;		//!< Time spent updating diffuse particles
	float updateTriangles;		//!< Time spent updating dynamic triangles
	float updateNormals;		//!< Time spent updating vertex normals
	float finalize;				//!< Time spent finalizing state
	float updateBounds;			//!< Time spent updating particle bounds
	float total;				//!< Sum of all timers above
};

/**
 * Flex error return codes
 */
enum NvFlexErrorSeverity
{
	eNvFlexLogError		=  0,	//!< Error messages
	eNvFlexLogInfo		=  1,	//!< Information messages
	eNvFlexLogWarning	=  2,	//!< Warning messages
	eNvFlexLogDebug		=  4,	//!< Used only in debug version of dll
	eNvFlexLogAll		= -1,	//!< All log types
};

 
/** Defines the set of stages at which callbacks may be registered 
 */
enum NvFlexSolverCallbackStage
{
	eNvFlexStageIterationStart,	//!< Called at the beginning of each constraint iteration
	eNvFlexStageIterationEnd,	//!< Called at the end of each constraint iteration
	eNvFlexStageSubstepBegin,	//!< Called at the beginning of each substep after the prediction step has been completed
	eNvFlexStageSubstepEnd,		//!< Called at the end of each substep after the velocity has been updated by the constraints
	eNvFlexStageUpdateEnd,		//!< Called at the end of solver update after the final substep has completed
	eNvFlexStageCount,			//!< Number of stages
};

/** Defines the different DirectX compute modes that Flex can use
*/
enum NvFlexComputeType
{
	eNvFlexCUDA,		//!< Use CUDA compute for Flex, the application must link against the CUDA libraries
	eNvFlexD3D11,		//!< Use DirectX 11 compute for Flex, the application must link against the D3D libraries
	eNvFlexD3D12,		//!< Use DirectX 12 compute for Flex, the application must link against the D3D libraries
};

/** Structure containing pointers to the internal solver data that is passed to each registered solver callback
 *
 *  @remarks Pointers to internal data are only valid for the lifetime of the callback and should not be stored.
 *  However, it is safe to launch kernels and memory transfers using the device pointers.
 *
 *  @remarks Because Flex re-orders particle data internally for performance, the particle data in the callback is not
 *  in the same order as it was provided to the API. The callback provides arrays which map original particle indices
 *  to sorted positions and vice-versa.
 *
 *  @remarks Particle positions may be modified during any callback, but velocity modifications should only occur during 
 *  the eNvFlexStageUpdateEnd stage, otherwise any velocity changes will be discarded.
 */
struct NvFlexSolverCallbackParams
{
	NvFlexSolver* solver;				//!< Pointer to the solver that the callback is registered to
	void* userData;						//!< Pointer to the user data provided to NvFlexRegisterSolverCallback()

	float* particles;					//!< Device pointer to the active particle basic data in the form x,y,z,1/m
	float* velocities;					//!< Device pointer to the active particle velocity data in the form x,y,z,w (last component is not used)
	int* phases;						//!< Device pointer to the active particle phase data

	int numActive;						//!< The number of active particles returned, the callback data only return pointers to active particle data, this is the same as NvFlexGetActiveCount()
	
	float dt;							//!< The per-update time-step, this is the value passed to NvFlexUpdateSolver()

	const int* originalToSortedMap;		//!< Device pointer that maps the sorted callback data to the original position given by SetParticles()
	const int* sortedToOriginalMap;		//!< Device pointer that maps the original particle index to the index in the callback data structure
};

/** Descriptor used to initialize Flex
*/
struct NvFlexInitDesc
{
	int deviceIndex;				//!< The GPU device index that should be used, if there is already a CUDA context on the calling thread then this parameter will be ignored and the active CUDA context used. Otherwise a new context will be created using the suggested device ordinal.
	bool enableExtensions;			//!< Enable or disable NVIDIA/AMD extensions in DirectX, can lead to improved performance.
	void* renderDevice;				//!< Direct3D device to use for simulation, if none is specified a new device and context will be created.
	void* renderContext;			//!< Direct3D context to use for simulation, if none is specified a new context will be created, in DirectX 12 this should be a pointer to the ID3D12CommandQueue where compute operations will take place. 
	
	NvFlexComputeType computeType;	//!< Set to eNvFlexD3D11 if DirectX 11 should be used, eNvFlexD3D12 for DirectX 12, this must match the libraries used to link the application
};

/** Solver callback definition, see NvFlexRegisterSolverCallback()
 */
struct NvFlexSolverCallback
{
	/** User data passed to the callback*/
	void* userData;
	
	/** Function pointer to a callback method */
	void (*function)(NvFlexSolverCallbackParams params);
};

/**
 * Function pointer type for error reporting callbacks
 */
typedef void (*NvFlexErrorCallback)(NvFlexErrorSeverity type, const char* msg, const char* file, int line);

/**
* Initialize library, should be called before any other API function.
*
*
* @param[in] version The version number the app is expecting, should almost always be NV_FLEX_VERSION
* @param[in] errorFunc The callback used for reporting errors.
* @param[in] desc The NvFlexInitDesc struct defining the device ordinal, D3D device/context and the type of D3D compute being used
* @return A pointer to a library instance that can be used to allocate shared object such as triangle meshes, buffers, etc
*/
NV_FLEX_API NvFlexLibrary* NvFlexInit(int version = NV_FLEX_VERSION, NvFlexErrorCallback errorFunc = 0, NvFlexInitDesc * desc = 0);

/**
 * Shutdown library, users should manually destroy any previously created   
 * solvers to ensure memory is freed before calling this method. If a new CUDA context was created during NvFlexInit() then it will be destroyed.
 *
 * @param[in] lib The library intance to use
 */
NV_FLEX_API void NvFlexShutdown(NvFlexLibrary* lib);

/**
 * Get library version number
 */
NV_FLEX_API int NvFlexGetVersion();

/**
 * Create a new particle solver
 *
 * @param[in] lib The library instance to use
 * @param[in] maxParticles Maximum number of simulation particles possible for this solver
 * @param[in] maxDiffuseParticles Maximum number of diffuse (non-simulation) particles possible for this solver
 * @param[in] maxNeighborsPerParticle Maximum number of neighbors per particle possible for this solver
 */
NV_FLEX_API NvFlexSolver* NvFlexCreateSolver(NvFlexLibrary* lib, int maxParticles, int maxDiffuseParticles, int maxNeighborsPerParticle = 96);
/**
 * Delete a particle solver
 *
 * @param[in] solver A valid solver pointer created from NvFlexCreateSolver()
 */
NV_FLEX_API void NvFlexDestroySolver(NvFlexSolver* solver);

/**
 * Return the library associated with a solver
 *
 * @param[in] solver A valid solver created with NvFlexCreateSolver()
 * @return A library pointer
 */
NV_FLEX_API NvFlexLibrary* NvFlexGetSolverLibrary(NvFlexSolver* solver);

/** Registers a callback for a solver stage, the callback will be invoked from the same thread that calls NvFlexUpdateSolver().
 *
 * @param[in] solver A valid solver
 * @param[in] function A pointer to a function that will be called during the solver update
 * @param[in] stage The stage of the update at which the callback function will be called
 *
 * @return The previously registered callback for this slot, this allows multiple users to chain callbacks together
 */
NV_FLEX_API NvFlexSolverCallback NvFlexRegisterSolverCallback(NvFlexSolver* solver, NvFlexSolverCallback function, NvFlexSolverCallbackStage stage);

/**
 * Integrate particle solver forward in time. Below is an example of how to step Flex in the context of a simple game loop:
 *
 \code{.c}
	
	NvFlexLibrary* library = NvFlexInit();
	NvFlexSolver* solver = NvFlexCreateSolver(library);

	NvFlexBuffer* particleBuffer = NvFlexAllocBuffer(library, n, sizeof(Vec4), eNvFlexBufferHost);
	NvFlexBuffer* velocityBuffer = NvFlexAllocBuffer(library, n, sizeof(Vec4), eNvFlexBufferHost);
	NvFlexBuffer* phaseBuffer = NvFlexAllocBuffer(library, n, sizeof(int), eNvFlexBufferHost);

	while(!done)
	{
		// map buffers for reading / writing
		float4* particles = (float4*)NvFlexMap(particles, eNvFlexMapWait);
		float3* velocities  = (float3*)NvFlexMap(velocities, eNvFlexMapWait);
		int* phases = (int*)NvFlexMap(phases, eNvFlexMapWait);

		// spawn (user method)
		SpawnParticles(particles, velocities, phases);

		// render (user method)
		RenderParticles(particles, velocities, phases);

		// unmap buffers
		NvFlexUnmap(particleBuffer);
		NvFlexUnmap(velocityBuffer);
		NvFlexUnmap(phaseBuffer);

		// write to device (async)
		NvFlexSetParticles(particleBuffer, n);
		NvFlexSetVelocities(velocityBuffer, n);
		NvFlexSetPhases(phaseBuffer, n);

		// tick
		NvFlexUpdateSolver(solver, dt, 1, NULL);

		// read back (async)
		NvFlexGetParticles(particleBuffer, n);
		NvFlexGetVelocities(velocityBuffer, n);
		NvFlexGetPhases(phaseBuffer, n);
	}

	NvFlexFreeBuffer(particleBuffer);
	NvFlexFreeBuffer(velocityBuffer);
	NvFlexFreeBuffer(phaseBuffer);

	NvFlexDestroySolver(solver);
	NvFlexShutdown(library);


 \endcode
 *
 * @param[in] solver A valid solver
 * @param[in] dt Time to integrate the solver forward in time by
 * @param[in] substeps The time dt will be divided into the number of sub-steps given by this parameter
 * @param[in] enableTimers Whether to enable per-kernel timers for profiling. Note that profiling can substantially slow down overall performance so this param should only be true in non-release builds
 */
NV_FLEX_API void NvFlexUpdateSolver(NvFlexSolver* solver, float dt, int substeps, bool enableTimers);

/**
 * Update solver paramters
 *
 * @param[in] solver A valid solver
 * @param[in] params Parameters structure in host memory, see NvFlexParams
 */
NV_FLEX_API void NvFlexSetParams(NvFlexSolver* solver, const NvFlexParams* params);

/**
 * Retrieve solver paramters, default values will be set at solver creation time
 *
 * @param[in] solver A valid solver
 * @param[out] params Parameters structure in host memory, see NvFlexParams
 */

NV_FLEX_API void NvFlexGetParams(NvFlexSolver* solver, NvFlexParams* params);

/**
 * Set the active particles indices in the solver
 * 
 * @param[in] solver A valid solver
 * @param[in] indices Holds the indices of particles that have been made active
 * @param[in] n Number of particles to allocate
 */
NV_FLEX_API void NvFlexSetActive(NvFlexSolver* solver, NvFlexBuffer* indices, int n);

/**
 * Return the active particle indices
 * 
 * @param[in] solver A valid solver
 * @param[out] indices a buffer of indices at least activeCount in length
 */
NV_FLEX_API void NvFlexGetActive(NvFlexSolver* solver, NvFlexBuffer* indices);

/**
 * Return the number of active particles in the solver
 * 
 * @param[in] solver A valid solver
 * @return The number of active particles in the solver
 */
NV_FLEX_API int NvFlexGetActiveCount(NvFlexSolver* solver);

/**
 * Set the particles state of the solver, a particle consists of 4 floating point numbers, its x,y,z position followed by its inverse mass (1/m)
 * 
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of particle data, should be 4*n in length
 * @param[in] n The number of particles to set
 *
 */
NV_FLEX_API void NvFlexSetParticles(NvFlexSolver* solver, NvFlexBuffer* p, int n);

/**
 * Get the particles state of the solver, a particle consists of 4 floating point numbers, its x,y,z position followed by its inverse mass (1/m)
 * 
 * @param[in] solver A valid solver
 * @param[out] p Pointer to a buffer of 4*n floats that will be filled out with the particle data, can be either a host or device pointer
 * @param[in] n The number of particles to get, must be less than max particles passed to NvFlexCreateSolver
 */
NV_FLEX_API void NvFlexGetParticles(NvFlexSolver* solver, NvFlexBuffer* p, int n);

/**
 * Set the particle positions in their rest state, if eNvFlexPhaseSelfCollideFilter is set on the particle's
 * phase attribute then particles that overlap in the rest state will not generate collisions with each other
 * 
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of particle data, should be 4*n in length
 * @param[in] n The number of particles to set
 *
 */
NV_FLEX_API void NvFlexSetRestParticles(NvFlexSolver* solver, NvFlexBuffer* p, int n);

/**
 * Get the particle positions in their rest state
 * 
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of particle data, should be 4*n in length
 * @param[in] n The number of particles to set
 *
 */
NV_FLEX_API void NvFlexGetRestParticles(NvFlexSolver* solver, NvFlexBuffer* p, int n);


/**
 * Get the Laplacian smoothed particle positions for rendering, see NvFlexParams::smoothing
 * 
 * @param[in] solver A valid solver
 * @param[out] p Pointer to a buffer of 4*n floats that will be filled out with the data, can be either a host or device pointer
 * @param[in] n The number of smooth particles to return
 */
NV_FLEX_API void NvFlexGetSmoothParticles(NvFlexSolver* solver, NvFlexBuffer*  p, int n);

/**
 * Set the particle velocities, each velocity is a 3-tuple of x,y,z floating point values
 * 
 * @param[in] solver A valid solver
 * @param[in] v Pointer to a buffer of 3*n floats
 * @param[in] n The number of velocities to set
 *
 */
NV_FLEX_API void NvFlexSetVelocities(NvFlexSolver* solver, NvFlexBuffer*  v, int n);
/**
 * Get the particle velocities, each velocity is a 3-tuple of x,y,z floating point values
 * 
 * @param[in] solver A valid solver
 * @param[out] v Pointer to a buffer of 3*n floats that will be filled out with the data, can be either a host or device pointer
 * @param[in] n The number of velocities to get
 */
NV_FLEX_API void NvFlexGetVelocities(NvFlexSolver* solver, NvFlexBuffer*  v, int n);

/**
 * Set the particles phase id array, each particle has an associated phase id which 
 * controls how it interacts with other particles. Particles with phase 0 interact with all
 * other phase types.
 *
 * Particles with a non-zero phase id only interact with particles whose phase differs 
 * from theirs. This is useful, for example, to stop particles belonging to a single
 * rigid shape from interacting with each other.
 * 
 * Phase 0 is used to indicate fluid particles when NvFlexParams::mFluid is set.
 * 
 * @param[in] solver A valid solver
 * @param[in] phases Pointer to a buffer of n integers containing the phases
 * @param[in] n The number of phases to set
 *
 */
NV_FLEX_API void NvFlexSetPhases(NvFlexSolver* solver, NvFlexBuffer* phases, int n);
/**
 * Get the particle phase ids
 * 
 * @param[in] solver A valid solver
 * @param[out] phases Pointer to a buffer of n integers that will be filled with the phase data, can be either a host or device pointer
 * @param[in] n The number of phases to get
 */
NV_FLEX_API void NvFlexGetPhases(NvFlexSolver* solver, NvFlexBuffer* phases, int n);

/**
 * Set per-particle normals to the solver, these will be overwritten after each simulation step, but can be used to initialize the normals to valid values
 * 
 * @param[in] solver A valid solver
 * @param[in] normals Pointer to a buffer of normals, should be 4*n in length
 * @param[in] n The number of normals to set
 *
 */
NV_FLEX_API void NvFlexSetNormals(NvFlexSolver* solver, NvFlexBuffer* normals, int n);

/**
 * Get per-particle normals from the solver, these are the world-space normals computed during surface tension, cloth, and rigid body calculations
 * 
 * @param[in] solver A valid solver
 * @param[out] normals Pointer to a buffer of normals, should be 4*n in length
 * @param[in] n The number of normals to get
 */
NV_FLEX_API void NvFlexGetNormals(NvFlexSolver* solver, NvFlexBuffer* normals, int n);


/**
 * Set distance constraints for the solver. Each distance constraint consists of two particle indices
 * stored consecutively, a rest-length, and a stiffness value. These are not springs in the traditional
 * sense, but behave somewhat like a traditional spring when lowering the stiffness coefficient.
 * 
 * @param[in] solver A valid solver
 * @param[in] indices Pointer to the spring indices array, should be 2*numSprings length, 2 indices per-spring
 * @param[in] restLengths Pointer to a buffer of rest lengths, should be numSprings length
 * @param[in] stiffness Pointer to the spring stiffness coefficents, should be numSprings in length, a negative stiffness value represents a tether constraint
 * @param[in] numSprings The number of springs to set
 *
 */
NV_FLEX_API void NvFlexSetSprings(NvFlexSolver* solver, NvFlexBuffer* indices, NvFlexBuffer* restLengths, NvFlexBuffer* stiffness, int numSprings);
/**
 * Get the distance constraints from the solver
 * 
 * @param[in] solver A valid solver
 * @param[out] indices Pointer to the spring indices array, should be 2*numSprings length, 2 indices per-spring
 * @param[out] restLengths Pointer to a buffer of rest lengths, should be numSprings length
 * @param[out] stiffness Pointer to the spring stiffness coefficents, should be numSprings in length, a negative stiffness value represents a unilateral tether constraint (only resists stretching, not compression), valid range [-1, 1]
 * @param[in] numSprings The number of springs to get
 */
NV_FLEX_API void NvFlexGetSprings(NvFlexSolver* solver, NvFlexBuffer* indices, NvFlexBuffer* restLengths, NvFlexBuffer* stiffness, int numSprings);

/**
 * Set rigid body constraints for the solver. 
 * @note A particle should not belong to more than one rigid body at a time.
 * 
 * @param[in] solver A valid solver
 * @param[in] offsets Pointer to a buffer of start offsets for a rigid in the indices array, should be numRigids+1 in length, the first entry must be 0
 * @param[in] indices Pointer to a buffer of indices for the rigid bodies, the indices for the jth rigid body start at indices[offsets[j]] and run to indices[offsets[j+1]] exclusive
 * @param[in] restPositions Pointer to a buffer of local space positions relative to the rigid's center of mass (average position), this should be at least 3*numIndices in length in the format x,y,z
 * @param[in] restNormals Pointer to a buffer of local space normals, this should be at least 4*numIndices in length in the format x,y,z,w where w is the (negative) signed distance of the particle inside its shape
 * @param[in] stiffness Pointer to a buffer of rigid stiffness coefficents, should be numRigids in length, valid values in range [0, 1]
 * @param[in] rotations Pointer to a buffer of quaternions (4*numRigids in length)
 * @param[in] translations Pointer to a buffer of translations of the center of mass (3*numRigids in length)
 * @param[in] numRigids The number of rigid bodies to set
 * @param[in] numIndices The number of indices in the indices array
 *
 */
NV_FLEX_API void NvFlexSetRigids(NvFlexSolver* solver, NvFlexBuffer* offsets, NvFlexBuffer* indices, NvFlexBuffer* restPositions, NvFlexBuffer* restNormals, NvFlexBuffer* stiffness, NvFlexBuffer* rotations, NvFlexBuffer* translations, int numRigids, int numIndices);


/**
 * Get the rotation matrices for the rigid bodies in the solver
 * 
 * @param[in] solver A valid solver
 * @param[out] rotations Pointer to a buffer of quaternions, should be 4*numRigids floats in length
 * @param[out] translations Pointer to a buffer of vectors to hold the rigid translations, should be 3*numRigids floats in length
 */
NV_FLEX_API void NvFlexGetRigidTransforms(NvFlexSolver* solver, NvFlexBuffer* rotations, NvFlexBuffer* translations);

/**
 * An opaque type representing a static triangle mesh in the solver
 */
typedef unsigned int NvFlexTriangleMeshId;

/**
 * An opaque type representing a signed distance field collision shape in the solver.
 */
typedef unsigned int NvFlexDistanceFieldId;

/**
 * An opaque type representing a convex mesh collision shape in the solver.
 * Convex mesh shapes may consist of up to 64 planes of the form a*x + b*y + c*z + d = 0,
 * particles will be constrained to the outside of the shape.
 */
typedef unsigned int NvFlexConvexMeshId;

/**
 * Create triangle mesh geometry, note that meshes may be used by multiple solvers if desired
 * 
 * @param[in] lib The library instance to use
 * @return A pointer to a triangle mesh object
 */
NV_FLEX_API NvFlexTriangleMeshId NvFlexCreateTriangleMesh(NvFlexLibrary* lib);

/**
 * Destroy a triangle mesh created with NvFlexCreateTriangleMesh()
 *
 * @param[in] lib The library instance to use
 * @param[in] mesh A triangle mesh created with NvFlexCreateTriangleMesh()
 */
NV_FLEX_API void NvFlexDestroyTriangleMesh(NvFlexLibrary* lib, NvFlexTriangleMeshId mesh);

/**
 * Specifies the triangle mesh geometry (vertices and indices), this method will cause any internal
 * data structures (e.g.: bounding volume hierarchies) to be rebuilt.
 * 
 * @param[in] lib The library instance to use
 * @param[in] mesh A triangle mesh created with NvFlexCreateTriangleMesh()
 * @param[in] vertices Pointer to a buffer of float3 vertex positions
 * @param[in] indices Pointer to a buffer of triangle indices, should be length numTriangles*3
 * @param[in] numVertices The number of vertices in the vertices array
 * @param[in] numTriangles The number of triangles in the mesh
 * @param[in] lower A pointer to a float3 vector holding the lower spatial bounds of the mesh
 * @param[in] upper A pointer to a float3 vector holding the upper spatial bounds of the mesh
 */
NV_FLEX_API void NvFlexUpdateTriangleMesh(NvFlexLibrary* lib, NvFlexTriangleMeshId mesh, NvFlexBuffer* vertices, NvFlexBuffer* indices, int numVertices, int numTriangles, const float* lower, const float* upper);

/**
 * Retrieve the local space bounds of the mesh, these are the same values specified to NvFlexUpdateTriangleMesh()
 * 
 * @param[in] lib The library instance to use
 * @param[in] mesh Pointer to a triangle mesh object
 * @param[out] lower Pointer to a buffer of 3 floats that the lower mesh bounds will be written to
 * @param[out] upper Pointer to a buffer of 3 floats that the upper mesh bounds will be written to
 */
NV_FLEX_API void NvFlexGetTriangleMeshBounds(NvFlexLibrary* lib, const NvFlexTriangleMeshId mesh, float* lower, float* upper);

/**
 * Create a signed distance field collision shape, see NvFlexDistanceFieldId for details.
 * 
 * @param[in] lib The library instance to use
 * @return A pointer to a signed distance field object
 */
NV_FLEX_API NvFlexDistanceFieldId NvFlexCreateDistanceField(NvFlexLibrary* lib);

/**
 * Destroy a signed distance field
 * 
 * @param[in] lib The library instance to use
 * @param[in] sdf A signed distance field created with NvFlexCreateDistanceField()
 */
NV_FLEX_API void NvFlexDestroyDistanceField(NvFlexLibrary* lib, NvFlexDistanceFieldId sdf);

/**
 * Update the signed distance field volume data, this method will upload
 * the field data to a 3D texture on the GPU
 * 
 * @param[in] lib The library instance to use
 * @param[in] sdf A signed distance field created with NvFlexCreateDistanceField()
 * @param[in] dimx The x-dimension of the volume data in voxels
 * @param[in] dimy The y-dimension of the volume data in voxels
 * @param[in] dimz The z-dimension of the volume data in voxels
 * @param[in] field The volume data stored such that the voxel at the x,y,z coordinate is addressed as field[z*dimx*dimy + y*dimx + x]
 */
NV_FLEX_API void NvFlexUpdateDistanceField(NvFlexLibrary* lib, NvFlexDistanceFieldId sdf, int dimx, int dimy, int dimz, NvFlexBuffer* field);

/**
 * Create a convex mesh collision shapes, see NvFlexConvexMeshId for details.
 * 
 * @param[in] lib The library instance to use
 * @return A pointer to a signed distance field object
 */
NV_FLEX_API NvFlexConvexMeshId NvFlexCreateConvexMesh(NvFlexLibrary* lib);

/**
 * Destroy a convex mesh
 * 
 * @param[in] lib The library instance to use
 * @param[in] convex A a convex mesh created with NvFlexCreateConvexMesh()
 */
NV_FLEX_API void NvFlexDestroyConvexMesh(NvFlexLibrary* lib, NvFlexConvexMeshId convex);

/**
 * Update the convex mesh geometry
 * 
 * @param[in] lib The library instance to use
 * @param[in] convex A valid convex mesh shape created from NvFlexCreateConvexMesh()
 * @param[in] planes An array of planes, each plane consists of 4 floats in the form a*x + b*y + c*z + d = 0
 * @param[in] numPlanes The number of planes in the convex
 * @param[in] lower The local space lower bound of the convex shape
 * @param[in] upper The local space upper bound of the convex shape
  */
NV_FLEX_API void NvFlexUpdateConvexMesh(NvFlexLibrary* lib, NvFlexConvexMeshId convex, NvFlexBuffer* planes, int numPlanes, float* lower, float* upper);

/**
 * Retrieve the local space bounds of the mesh, these are the same values specified to NvFlexUpdateConvexMesh()
 * 
 * @param[in] lib The library instance to use
 * @param[in] mesh Pointer to a convex mesh object
 * @param[out] lower Pointer to a buffer of 3 floats that the lower mesh bounds will be written to
 * @param[out] upper Pointer to a buffer of 3 floats that the upper mesh bounds will be written to
 */
NV_FLEX_API void NvFlexGetConvexMeshBounds(NvFlexLibrary* lib, NvFlexConvexMeshId mesh, float* lower, float* upper);

/**
 * A basic sphere shape with origin at the center of the sphere and radius
 */
struct NvFlexSphereGeometry
{
	float radius;
};

/**
 * A collision capsule extends along the x-axis with its local origin at the center of the capsule 
 */
struct NvFlexCapsuleGeometry
{
	float radius;
	float halfHeight;
};

/**
 * A simple box with interior [-halfHeight, +halfHeight] along each dimension 
 */
struct NvFlexBoxGeometry
{
	float halfExtents[3];
};

/**
 * A convex mesh instance with non-uniform scale
 */
struct NvFlexConvexMeshGeometry
{
	float scale[3];
	NvFlexConvexMeshId mesh;
};

/**
 * A scaled triangle mesh instance with non-uniform scale
 */
struct NvFlexTriangleMeshGeometry
{
	float scale[3];			//!< The scale of the object from local space to world space
	NvFlexTriangleMeshId mesh;	//!< A triangle mesh pointer created by NvFlexCreateTriangleMesh()
};

/**
 * A scaled signed distance field instance, the local origin of the SDF is at corner of the field corresponding to the first voxel.
 * The field is mapped to the local space volume [0, 1] in each dimension.
 */
struct NvFlexSDFGeometry
{
	float scale;				 //!< Uniform scale of SDF, this corresponds to the world space width of the shape
	NvFlexDistanceFieldId field;	 //!< A signed distance field pointer created by NvFlexCreateDistanceField()
};

/**
 * This union allows collision geometry to be sent to Flex as a flat array of 16-byte data structures,
 * the shape flags array specifies the type for each shape, see NvFlexSetShapes().
 */
union NvFlexCollisionGeometry
{
	NvFlexSphereGeometry sphere;
	NvFlexCapsuleGeometry capsule;
	NvFlexBoxGeometry box;
	NvFlexConvexMeshGeometry convexMesh;
	NvFlexTriangleMeshGeometry triMesh;
	NvFlexSDFGeometry sdf;
};

enum NvFlexCollisionShapeType
{
	eNvFlexShapeSphere			= 0,		//!< A sphere shape, see FlexSphereGeometry
	eNvFlexShapeCapsule			= 1,		//!< A capsule shape, see FlexCapsuleGeometry
	eNvFlexShapeBox				= 2,		//!< A box shape, see FlexBoxGeometry
	eNvFlexShapeConvexMesh		= 3,		//!< A convex mesh shape, see FlexConvexMeshGeometry
	eNvFlexShapeTriangleMesh	= 4,		//!< A triangle mesh shape, see FlexTriangleMeshGeometry
	eNvFlexShapeSDF				= 5,		//!< A signed distance field shape, see FlexSDFGeometry
};

enum NvFlexCollisionShapeFlags
{
	eNvFlexShapeFlagTypeMask	= 0x7,		//!< Lower 3 bits holds the type of the collision shape
	eNvFlexShapeFlagDynamic		= 8,		//!< Indicates the shape is dynamic and should have lower priority over static collision shapes
	eNvFlexShapeFlagTrigger		= 16,		//!< Indicates that the shape is a trigger volume, this means it will not perform any collision response, but will be reported in the contacts array (see NvFlexGetContacts())

	eNvFlexShapeFlagReserved	= 0xffffff00
};

/** 
 * Combines geometry type and static/dynamic flags
 */
NV_FLEX_API inline int NvFlexMakeShapeFlags(NvFlexCollisionShapeType type, bool dynamic) { return type | (dynamic?eNvFlexShapeFlagDynamic:0); }

/**
 * Set the collision shapes for the solver
 * 
 * @param[in] solver A valid solver
 * @param[in] geometry Pointer to a buffer of NvFlexCollisionGeometry entries, the type of each shape determines how many entries it has in the array
 * @param[in] shapePositions Pointer to a buffer of translations for each shape in world space, should be 4*numShapes in length
 * @param[in] shapeRotations Pointer to an a buffer of rotations for each shape stored as quaternion, should be 4*numShapes in length
 * @param[in] shapePrevPositions Pointer to a buffer of translations for each shape at the start of the time step, should be 4*numShapes in length
 * @param[in] shapePrevRotations Pointer to an a buffer of rotations for each shape stored as a quaternion at the start of the time step, should be 4*numShapees in length
 * @param[in] shapeFlags The type and behavior of the shape, NvFlexCollisionShapeFlags for more detail
 * @param[in] numShapes The number of shapes
 *
 */
NV_FLEX_API void NvFlexSetShapes(NvFlexSolver* solver, NvFlexBuffer* geometry, NvFlexBuffer* shapePositions, NvFlexBuffer* shapeRotations, NvFlexBuffer* shapePrevPositions, NvFlexBuffer* shapePrevRotations, NvFlexBuffer* shapeFlags, int numShapes);

/**
 * Set dynamic triangles mesh indices, typically used for cloth. Flex will calculate normals and 
 * apply wind and drag effects to connected particles. See NvFlexParams::drag, NvFlexParams::wind.
 * 
 * @param[in] solver A valid solver
 * @param[in] indices Pointer to a buffer of triangle indices into the particles array, should be 3*numTris in length
 * @param[in] normals Pointer to a buffer of triangle normals, should be 3*numTris in length, can be NULL
 * @param[in] numTris The number of dynamic triangles
 *s
 */
NV_FLEX_API void NvFlexSetDynamicTriangles(NvFlexSolver* solver, NvFlexBuffer* indices, NvFlexBuffer* normals, int numTris);
/**
 * Get the dynamic triangle indices and normals.
 * 
 * @param[in] solver A valid solver
 * @param[out] indices Pointer to a buffer of triangle indices into the particles array, should be 3*numTris in length, if NULL indices will not be returned
 * @param[out] normals Pointer to a buffer of triangle normals, should be 3*numTris in length, if NULL normals will be not be returned
 * @param[in] numTris The number of dynamic triangles
 */
NV_FLEX_API void NvFlexGetDynamicTriangles(NvFlexSolver* solver, NvFlexBuffer* indices, NvFlexBuffer* normals, int numTris);

/**
 * Set inflatable shapes, an inflatable is a range of dynamic triangles (wound CCW) that represent a closed mesh.
 * Each inflatable has a given rest volume, constraint scale (roughly equivalent to stiffness), and "over pressure"
 * that controls how much the shape is inflated.
 * 
 * @param[in] solver A valid solver
 * @param[in] startTris Pointer to a buffer of offsets into the solver's dynamic triangles for each inflatable, should be numInflatables in length
 * @param[in] numTris Pointer to a buffer of triangle counts for each inflatable, should be numInflatablesin length
 * @param[in] restVolumes Pointer to a buffer of rest volumes for the inflatables, should be numInflatables in length
 * @param[in] overPressures Pointer to a buffer of floats specifying the pressures for each inflatable, a value of 1.0 means the rest volume, > 1.0 means over-inflated, and < 1.0 means under-inflated, should be numInflatables in length
 * @param[in] constraintScales Pointer to a buffer of scaling factors for the constraint, this is roughly equivalent to stiffness but includes a constraint scaling factor from position-based dynamics, see helper code for details, should be numInflatables in length
 * @param[in] numInflatables Number of inflatables to set
 *
 */
NV_FLEX_API void NvFlexSetInflatables(NvFlexSolver* solver, NvFlexBuffer* startTris, NvFlexBuffer* numTris, NvFlexBuffer* restVolumes, NvFlexBuffer* overPressures, NvFlexBuffer* constraintScales, int numInflatables);

/**
 * Get the density values for fluid particles
 *
 * @param[in] solver A valid solver
 * @param[in] n The number of particle densities to return
 * @param[out] densities Pointer to a buffer of floats, should be maxParticles in length, density values are normalized between [0, 1] where 1 represents the rest density
 */
NV_FLEX_API void NvFlexGetDensities(NvFlexSolver* solver, NvFlexBuffer* densities, int n);

/**
 * Get the anisotropy of fluid particles, the particle distribution for a particle is represented
 * by 3 orthogonal vectors. Each 3-vector has unit length with the variance along that axis
 * packed into the w component, i.e.: x,y,z,lambda.
*
 * The anisotropy defines an oriented ellipsoid in worldspace that can be used for rendering
 * or surface extraction.
 *
 * @param[in] solver A valid solver
 * @param[out] q1 Pointer to a buffer of floats that receive the first basis vector and scale, should be 4*maxParticles in length
 * @param[out] q2 Pointer to a buffer of floats that receive the second basis vector and scale, should be 4*maxParticles in length
 * @param[out] q3 Pointer to a buffer of floats that receive the third basis vector and scale, should be 4*maxParticles in length
 */
NV_FLEX_API void NvFlexGetAnisotropy(NvFlexSolver* solver, NvFlexBuffer* q1, NvFlexBuffer* q2, NvFlexBuffer* q3);
/**
 * Get the state of the diffuse particles. Diffuse particles are passively advected by the fluid
 * velocity field.
 *
 * @param[in] solver A valid solver
 * @param[out] p Pointer to a buffer of floats, should be 4*maxParticles in length, the w component represents the particles lifetime with 1 representing a new particle, and 0 representing an inactive particle
 * @param[out] v Pointer to a buffer of floats, should be 4*maxParticles in length, the w component is not used
 * @param[out] indices Pointer to a buffer of ints that specify particle indices in depth sorted order, should be maxParticles in length, see NvFlexParams::mDiffuseSortDir
 */
NV_FLEX_API int NvFlexGetDiffuseParticles(NvFlexSolver* solver, NvFlexBuffer* p, NvFlexBuffer* v, NvFlexBuffer* indices);

/**
 * Set the state of the diffuse particles. Diffuse particles are passively advected by the fluid
 * velocity field.
 *
 * @param[in] solver A valid solver
 * @param[in] p Pointer to a buffer of floats, should be 4*n in length, the w component represents the particles lifetime with 1 representing a new particle, and 0 representing an inactive particle
 * @param[in] v Pointer to a buffer of floats, should be 4*n in length, the w component is not used
 * @param[in] n Number of diffuse particles to set
 *
 */
NV_FLEX_API void NvFlexSetDiffuseParticles(NvFlexSolver* solver, NvFlexBuffer* p, NvFlexBuffer* v, int n);

/**
 * Get the particle contact planes. Note this will only include contacts that were active on the last substep of an update, and will include all contact planes generated within NvFlexParams::shapeCollisionMargin.
 *
 * @param[in] solver A valid solver
 * @param[out] planes Pointer to a destination buffer containing the contact planes for the particle, each particle can have up to 4 contact planes so this buffer should be 16*maxParticles in length
 * @param[out] velocities Pointer to a destination buffer containing the velocity of the contact point on the shape in world space, the index of the shape (corresponding to the shape in NvFlexSetShapes() is stored in the w component), each particle can have up to 4 contact planes so this buffer should be 16*maxParticles in length
 * @param[out] indices Pointer to a buffer of indices into the contacts buffer, the first contact plane for the i'th particle is given by planes[indices[i]*sizeof(float)*4] and subsequent contacts for that particle are stored sequentially, this array should be maxParticles in length
 * @param[out] counts Pointer to a buffer of contact counts for each particle (will be <= 4), this buffer should be maxParticles in length
 */
NV_FLEX_API void NvFlexGetContacts(NvFlexSolver* solver, NvFlexBuffer* planes, NvFlexBuffer* velocities, NvFlexBuffer* indices, NvFlexBuffer* counts);

/**
 * Get the world space AABB of all particles in the solver, note that the bounds are calculated during the update (see NvFlexUpdateSolver()) so only become valid after an update has been performed.
 * The returned bounds represent bounds of the particles in their predicted positions *before* the constraint solve.
 * 
 * @param[in] solver A valid solver
 * @param[out] lower Pointer to a buffer of 3 floats to receive the lower bounds
 * @param[out] upper Pointer to a buffer of 3 floats to receive the upper bounds
 */
NV_FLEX_API void NvFlexGetBounds(NvFlexSolver* solver, NvFlexBuffer* lower, NvFlexBuffer* upper);

/**
 *
 * @param[in] solver A valid solver
 * @return The time in seconds between the first and last GPU operations executed by the last NvFlexUpdateSolver.
 *
 * @note This method causes the CPU to wait until the GPU has finished any outstanding work. 
 *		 To avoid blocking the calling thread it should be called after work has completed, e.g.: directly after a NvFlexMap().
 */
NV_FLEX_API float NvFlexGetDeviceLatency(NvFlexSolver* solver);

/**
 * Fetch high-level GPU timers.
 *
 * @param[in] solver The solver instance to use
 * @param[out] timers A struct containing the GPU latency of each stage in the physics pipeline.
 *
 * @note This method causes the CPU to wait until the GPU has finished any outstanding work.
 *		 To avoid blocking the calling thread it should be called after work has completed, e.g.: directly after a NvFlexMap().
 *       To capture there timers you must pass true for enableTimers in NvFlexUpdateSolver()
 */
NV_FLEX_API void NvFlexGetTimers(NvFlexSolver* solver, NvFlexTimers* timers);

/**
* Holds the execution time for a specfic shader
*/
struct NvFlexDetailTimer
{ 
	char* name; 
	float time;
};

/**
* Fetch per-shader GPU timers.
*
* @param[in] solver The solver instance to use
* @param[out] timers An array of NvFlexDetailTimer structures, each representing a unique shader.
* @return The number of detail timers in the timers array
*
* @note This method causes the CPU to wait until the GPU has finished any outstanding work.
*		To avoid blocking the calling thread it should be called after work has completed, e.g.: directly after a NvFlexMap().
*       To capture there timers you must pass true for enableTimers in NvFlexUpdateSolver()
*		Timers are valid until the next call to NvFlexGetDetailTimers
*/
NV_FLEX_API int NvFlexGetDetailTimers(NvFlexSolver* solver, NvFlexDetailTimer** timers);

/**
 * Allocate a Flex buffer. Buffers are used to pass data to the API in an efficient manner.
 *
 * @param[in] lib The library instance to use
 * @param[in] elementCount The number of elements in the buffer
 * @param[in] elementByteStride The size of each element in bytes
 * @param[in] type The type of buffer to allocate, can be either host memory or device memory
 * @return A pointer to a NvFlexBuffer
 */
NV_FLEX_API NvFlexBuffer* NvFlexAllocBuffer(NvFlexLibrary* lib, int elementCount, int elementByteStride, NvFlexBufferType type);

/**
 * Free a Flex buffer
 *
 * @param[in] buf A buffer to free, must be allocated with NvFlexAllocBuffer()
 */
NV_FLEX_API void NvFlexFreeBuffer(NvFlexBuffer* buf);

/**
 * Maps a buffer for reading and writing. When the buffer is created with NvFlexBufferType::eHost, then the returned pointer will be a host memory address
 * that can be read/written.
 * Mapping a buffer implicitly synchronizes with the GPU to ensure that any reads or writes from the buffer (e.g.: from the NvFlexGet*() or NvFlexSet*()) methods have completed.
 *
 * @param[in] buffer A buffer allocated with NvFlexAllocBuffer()
 * @param[in] flags Hints to Flex how the buffer is to be accessed, typically this should be eNvFlexMapWait (0)
 * @return A pointer to the mapped memory
 */
NV_FLEX_API void* NvFlexMap(NvFlexBuffer* buffer, int flags);

/**
 * Unmaps a buffer that was mapped through NvFlexMap(), note that buffers must be unmapped before they can be passed to a NvFlexGet*() or NvFlexSet*() method
 *
 * @param[in] buffer A valid buffer allocated through NvFlexAllocBuffer()
 */
NV_FLEX_API void NvFlexUnmap(NvFlexBuffer* buffer);

/**
 * Registers an OpenGL buffer to Flex which can be used to copy directly into a graphics resource. Example usage is below
 *
 \code{.c}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW)

	NvFlexBuffer* vboBuffer = NvFlexRegisterOGLBuffer(lib, vbo, n, sizeof(float)*4);

	// simulate 
	...

	// copy directly from Flex into render buffer
	NvFlexGetParticles(vboBuffer, n);

	// render
	...

 \endcode
 *
 * @param[in] lib The library instance to use
 * @param[in] buf An OpenGL buffer identifier
 * @param[in] elementCount The number of elements in the buffer
 * @param[in] elementByteStride the size of each element in bytes
 * @return A valid NvFlexBuffer pointer that may be used with NvFlexGet*() methods to populate the render buffer using direct GPU-GPU copies
 */
NV_FLEX_API NvFlexBuffer* NvFlexRegisterOGLBuffer(NvFlexLibrary* lib, int buf, int elementCount, int elementByteStride);

/**
 * Unregister a NvFlexBuffer allocated through NvFlexRegisterOGLBuffer()
 *
 * @param[in] buf A valid buffer
 */
NV_FLEX_API void NvFlexUnregisterOGLBuffer(NvFlexBuffer* buf);

/**
* Registers a Direct3D buffer to Flex which can be used to copy directly into a graphics resource
*
* @param[in] lib The library instance to use
* @param[in] buffer A pointer to either an ID3D11Buffer or ID3D12Resource object
* @param[in] elementCount The number of elements in the buffer
* @param[in] elementByteStride the size of each element in bytes
* @return A valid NvFlexBuffer pointer that may be used with NvFlexGet*() methods to populate the render buffer using direct GPU-GPU copies
*/
NV_FLEX_API NvFlexBuffer* NvFlexRegisterD3DBuffer(NvFlexLibrary* lib, void* buffer, int elementCount, int elementByteStride);

/**
* Unregister a NvFlexBuffer allocated through NvFlexRegistereD3DBuffer()
*
* @param[in] buf A valid buffer
*/
NV_FLEX_API void NvFlexUnregisterD3DBuffer(NvFlexBuffer* buf);

/** 
 * Ensures that the CUDA context the library was initialized with is present on the current thread
 *
 * @param[in] lib The library instance to use
 */
NV_FLEX_API void NvFlexAcquireContext(NvFlexLibrary* lib);

/** 
 * Restores the CUDA context (if any) that was present on the last call to NvFlexAcquireContext()
 * Note: the acquire/restore pair of calls must come from the same thread
 */
NV_FLEX_API void NvFlexRestoreContext(NvFlexLibrary* lib);

/** 
 * Returns a null-terminated string with the compute device name
 *
 * @param[in] lib The library instance to use
 */
NV_FLEX_API const char* NvFlexGetDeviceName(NvFlexLibrary* lib);

/** 
 * Retrieve the device and context for the the library.
 * On CUDA the context pointer will be filled with a pointer to a CUcontext structure
 * On D3D the device and context pointers will be filled with pointers to a NvFlex::Device, and NvFlex::Context wrapper
 *
 * @param[in] lib Pointer to a valid library returned from NvFlexInit()
 * @param[out] device Pointer to a device pointer, see description
 * @param[out] context Pointer to a context pointer, see description
 */
NV_FLEX_API void NvFlexGetDeviceAndContext(NvFlexLibrary* lib, void** device, void** context);
 

/**
 * Force a pipeline flush to ensure any queued work is submitted to the GPU
 *
 * @param[in] lib The library instance to use
 */
NV_FLEX_API void NvFlexFlush(NvFlexLibrary* lib);

//! \cond HIDDEN_SYMBOLS

/**
 * Debug methods (unsupported)
 */

NV_FLEX_API void NvFlexSetDebug(NvFlexSolver* solver, bool enable);
NV_FLEX_API void NvFlexGetShapeBVH(NvFlexSolver* solver, void* bvh);
NV_FLEX_API void NvFlexCopySolver(NvFlexSolver* dst, NvFlexSolver* src);

//! \endcond

} // extern "C"

#endif // NV_FLEX_H
