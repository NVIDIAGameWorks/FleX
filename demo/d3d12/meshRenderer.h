
#ifndef MESH_RENDERER_H
#define MESH_RENDERER_H

#include "core/maths.h"

namespace FlexSample {

enum FrontWindingType
{
	FRONT_WINDING_CLOCKWISE,
	FRONT_WINDING_COUNTER_CLOCKWISE,
};

struct RenderAllocation;

enum PrimitiveType
{
	PRIMITIVE_UNKNOWN,
	PRIMITIVE_POINT,
	PRIMITIVE_LINE,
	PRIMITIVE_TRIANGLE,
};

/* Abstraction for how something is rendered. A pipeline indicates what kind of rendering it can be used with via the usage type */
struct RenderPipeline
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS_BASE(RenderPipeline);
public:
		/// Bind with platform specific state
	virtual int bind(const void* paramsIn, const void* platformState) = 0;
	virtual int draw(const RenderAllocation& pointAlloc, size_t sizeOfAlloc, const void* platformState) = 0;
	
		/// Get the usage type
	inline PrimitiveType getPrimitiveType() const { return m_primitiveType; }

		/// Ctor
	RenderPipeline(PrimitiveType primitiveType): m_primitiveType(primitiveType) {}

	virtual ~RenderPipeline() {}
	private:
	PrimitiveType m_primitiveType;
};

struct MeshData
{
	void init()
	{
		positions = nullptr;
		normals = nullptr;
		texcoords = nullptr;
		colors = nullptr;
		indices = nullptr;
		numFaces = 0;
		numVertices = 0;
	}

	const Vec3* positions;
    const Vec3* normals;
	const Vec2* texcoords;
	const Vec4* colors;
	const uint32_t* indices;
	int numVertices;
	int numFaces;
};

struct MeshData2
{
	void init()
	{
		positions = nullptr;
		normals = nullptr;
		texcoords = nullptr;
		colors = nullptr;
		indices = nullptr;
		numFaces = 0;
		numVertices = 0;
	}

	const Vec4* positions;
	const Vec4* normals;
	const Vec2* texcoords;
	const Vec4* colors;
	const uint32_t* indices;
	ptrdiff_t numVertices;
	ptrdiff_t numFaces;
};


struct LineData
{
	struct Vertex
	{
		Vec3 position;
		Vec4 color;
	};
	void init()
	{
		vertices = nullptr;
		indices = nullptr;
		numLines = 0;
		numVertices = 0;
	}
	const Vertex* vertices;				///< Must be set, and holds numPositions. If indices is nullptr, must hold at least numLines * 2 
	const uint32_t* indices;			///< If not nullptr holds 2 * numLines
	ptrdiff_t numVertices;					///< The total amount of positions
	ptrdiff_t numLines;					///< The total number of lines
};

struct PointData
{
	void init()
	{
		positions = nullptr;
		density = nullptr;
		phase = nullptr;
		numPoints = 0;
		for (int i = 0; i < 3; i++)
		{
			anisotropy[i] = nullptr;
		}
		indices = nullptr;
		numIndices = 0;
	}

	const Vec4* positions;
	const float* density;
	const int* phase;
	ptrdiff_t numPoints;					//< The number of values in position, density and phase. It must be +1 the maximum particle indexed

	const Vec4* anisotropy[3];	// Holds anisotropy or can be nullptr if not used

	const uint32_t* indices;				//< The indices to used particles
	ptrdiff_t numIndices;
};

struct RenderMesh
{	
	//NV_CO_DECLARE_POLYMORPHIC_CLASS_BASE(RenderMesh);
public:
	virtual ~RenderMesh() {};
};

struct RenderAllocation
{
	void init(PrimitiveType primType)
	{
		m_numPositions = -1;
		m_numPrimitives = -1;
		m_offset = 0;
		m_primitiveType = primType;
	}

	PrimitiveType m_primitiveType;					///< The primitive type to draw
	ptrdiff_t m_numPositions;						///< The total number of positions
	ptrdiff_t m_numPrimitives;						///< The total number of primitives
	ptrdiff_t m_offset;								///< The start location in the render buffer to offset
};

struct MeshRenderer
{
	//NV_CO_DECLARE_POLYMORPHIC_CLASS_BASE(MeshRenderer);
public:
		/// Draw a pre-created mesh
	virtual int draw(RenderMesh* mesh, RenderPipeline* pipeline, const void* params) = 0;

		/// Do an immediate mode draw
	virtual int drawImmediate(const MeshData& meshData, RenderPipeline* pipeline, const void* params) = 0;
		/// Do an immediate mode draw
	virtual int drawImmediate(const MeshData2& meshData, RenderPipeline* pipeline, const void* params) = 0;
		/// Draw particles immediately
	virtual int drawImmediate(const PointData& pointData, RenderPipeline* pipeline, const void* params) = 0;
		/// Draw lines immediately
	virtual int drawImmediate(const LineData& lineData, RenderPipeline* pipeline, const void* params) = 0;

		/// Render immediately using a previously transitory allocation
	virtual int drawTransitory(RenderAllocation& allocation, size_t sizeOfAlloc, RenderPipeline* pipeline, const void* params) = 0;

		/// Allocate rendering data temporarily in gpu accessible memory. Render with drawTransitory.
		/// A transitory allocations lifetime is dependent on rendering API, but typically stays in scope for a frame, so multiple
		/// draw Transitory allocation can be done for a single allocation - but only in drawing a single frame.
		/// NOTE! The PointAllocation/MeshAllocation structures must be the derived type for the API being used (say Dx12PointAllocation)
		/// this is verified by the sizeOfAlloc being that size.
	virtual int allocateTransitory(const PointData& pointData, RenderAllocation& allocation, size_t sizeOfAlloc) = 0;
	virtual int allocateTransitory(const MeshData& meshData, RenderAllocation& allocation, size_t sizeOfAlloc) = 0;
	virtual int allocateTransitory(const MeshData2& meshData, RenderAllocation& allocation, size_t sizeOfAlloc) = 0;
	virtual int allocateTransitory(const LineData& lineData, RenderAllocation& allocation, size_t sizeOfAlloc) = 0;

		/// Create a render mesh from mesh data
	virtual RenderMesh* createMesh(const MeshData& meshData) = 0;
	virtual RenderMesh* createMesh(const MeshData2& meshData) = 0;
};

} // namespace FlexSample

#endif