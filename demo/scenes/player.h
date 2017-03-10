

/*
class Player : public Scene
{ 
public:

	Player(const char* filename) : Scene("Player"), mFilename(filename), mRecording(NULL)
	{	
	}

	virtual void Initialize()
	{
		if (!mRecording)
			mRecording = fopen(mFilename, "rb");

		if (mRecording)
			fseek(mRecording, 0, SEEK_SET);

		// read first frame
		ReadFrame();

		g_lightDistance = 100.0f;
		g_fogDistance = 0.0f;

		g_camSpeed *= 100.0f;
		g_camNear *= 100.0f;
		g_camFar *= 100.0f;
		g_pause = true;
		
		g_dt = 1.0f/30.0f;
		g_numSubsteps = 2;

		g_drawPoints = true;

		mInitialActive = g_buffers->activeIndices;
	}

	virtual void PostInitialize()
	{
		g_buffers->activeIndices = mInitialActive;

		NvFlexSetActive(g_flex, &mInitialActive[0], mInitialActive.size(), eFlexMemoryHost);
	}

	virtual Matrix44 GetBasis()
	{
		// Coordinate fip for Unreal captures 

		Matrix44 flip = Matrix44::kIdentity;
		flip.SetCol(1, Vec4(0.0f, 0.0f, -1.0f, 0.0f));
		flip.SetCol(2, Vec4(0.0f, 1.0f, 0.0f, 0.0f));

		return flip;
	}

	template<typename Element>
	void ReadArray(std::vector<Element>& dest, bool enable=true)
	{
		if (feof(mRecording))
			return;

		int length;
		int r;
		r = fread(&length, sizeof(int), 1, mRecording);
			
		if (feof(mRecording))
			return;

		if (enable)
		{
			int numElements = length/sizeof(Element);

			dest.resize(numElements);
			r = fread(&dest[0], length, 1, mRecording);
		}
		else		
			r = fseek(mRecording, length, SEEK_CUR);

		(void)r;
	}

	virtual void KeyDown(int key)
	{
		if (key == '[')
		{
			Vec3 lower(FLT_MAX), upper(-FLT_MAX);
			
			// particle bounds
			for (int i=0; i < int(g_buffers->activeIndices.size()); ++i)
			{
				int index = g_buffers->activeIndices[i];

				lower = Min(Vec3(g_buffers->positions[index]), lower);
				upper = Max(Vec3(g_buffers->positions[index]), upper);
			}

			// center camera
			g_camPos = (lower+upper)*0.5f;
			g_camPos = GetBasis()*g_camPos;
		}
	}

	bool VerifyArray(float* ptr, int n)
	{
		for (int i=0; i < n; ++i)
			if (!isfinite(ptr[i]))
				return false;

		return true;
	}

	template <typename Element>
	void ReadValue(Element& e, bool enable=true)
	{
		if (feof(mRecording))
			return;

		int r;
		if (enable)
			r = fread(&e, sizeof(e), 1, mRecording);
		else
			r = fseek(mRecording, sizeof(e), SEEK_CUR);

		(void)r;
	}

	void ReadFrame()
	{
		if (!mRecording)
			return;

		if (feof(mRecording))
			return;

		// params
		ReadValue(g_params, true);

		// particle data
		//ReadArray(g_buffers->positions, true);

		if (true)
		{
			for (int i=0; i < int(g_buffers->positions.size()); ++i)
			{
				if (!isfinite(g_buffers->positions[i].x) || 
					!isfinite(g_buffers->positions[i].y) ||
					!isfinite(g_buffers->positions[i].z))
					printf("particles failed at frame %d\n", g_frame);
			}
		}

		ReadArray(g_buffers->restPositions, true);
		ReadArray(g_buffers->velocities, true);
		ReadArray(g_buffers->phases, true);
		ReadArray(g_buffers->activeIndices, true);

		// spring data
		ReadArray(g_buffers->springIndices, true);
		ReadArray(g_buffers->springLengths, true);
		ReadArray(g_buffers->springStiffness, true);

		// shape data
		ReadArray(g_buffers->rigidIndices, true);
		ReadArray(g_buffers->rigidLocalPositions, true);
		ReadArray(g_buffers->rigidLocalNormals, true);

		ReadArray(g_buffers->rigidCoefficients, true);
		ReadArray(g_buffers->rigidOffsets, true);
		ReadArray(g_buffers->rigidRotations, true);
		ReadArray(g_buffers->rigidTranslations, true);

		if (true)
		{

			if (!VerifyArray((float*)&g_buffers->rigidLocalPositions[0], g_buffers->rigidLocalPositions.size()*3))
				printf("rigid local pos failed\n");

			if (!VerifyArray((float*)&g_buffers->rigidTranslations[0], g_buffers->rigidTranslations.size()*3))
				printf("rigid translations failed\n");

			if (!VerifyArray((float*)&g_buffers->rigidRotations[0], g_buffers->rigidRotations.size()*3))
				printf("rigid rotations failed\n");
		}

		// triangle data
		ReadArray(g_buffers->triangles, true);
		ReadArray(g_buffers->triangleNormals, true);

		// convex shapes
		ReadArray(g_buffers->shapeGeometry, true);
		ReadArray(g_buffers->shapeAabbMin, true);
		ReadArray(g_buffers->shapeAabbMax, true);
		ReadArray(g_buffers->shapeStarts, true);		
		ReadArray(g_buffers->shapePositions, true);
		ReadArray(g_buffers->shapeRotations, true);
		ReadArray(g_buffers->shapePrevPositions, true);
		ReadArray(g_buffers->shapePrevRotations, true);
		ReadArray(g_buffers->shapeFlags, true);

		if (true)
		{
			if (!VerifyArray((float*)&g_buffers->shapePositions[0], g_buffers->shapePositions.size()*4))
				printf("shapes translations invalid\n");

			if (!VerifyArray((float*)&g_buffers->shapeRotations[0], g_buffers->shapeRotations.size()*4))
				printf("shapes rotations invalid\n");
		}

		int numMeshes = 0;
		ReadValue(numMeshes);

		// map serialized mesh ptrs to current meshes
		std::map<NvFlexTriangleMeshId, NvFlexTriangleMeshId> originalToNewMeshMap;

		for (int i=0; i < numMeshes; ++i)
		{
			Mesh m;

			NvFlexTriangleMeshId originalPtr;
			ReadValue(originalPtr);

			ReadArray(m.m_positions);
			ReadArray(m.m_indices);

			if (!VerifyArray((float*)&m.m_positions[0], m.m_positions.size()*3))
				printf("mesh vertices invalid\n");

			printf("Creating mesh: %d faces %d vertices\n", m.GetNumFaces(), m.GetNumVertices());

			Vec3 lower, upper;
			m.GetBounds(lower, upper);
			m.CalculateNormals();

			NvFlexTriangleMeshId collisionMesh = NvFlexCreateTriangleMesh();
			NvFlexUpdateTriangleMesh(collisionMesh, (float*)&m.m_positions[0], (int*)&m.m_indices[0], int(m.m_positions.size()), int(m.m_indices.size())/3, lower, upper, eFlexMemoryHost);

			// create a render mesh
			g_meshes[collisionMesh] = CreateGpuMesh(&m);

			// create map from captured triangle mesh pointer to our recreated version
			originalToNewMeshMap[originalPtr] = collisionMesh;
		}

		int numTriMeshInstances = 0;

		// remap shape ptrs
		for (int i=0; i < int(g_buffers->shapeFlags.size()); ++i)
		{
			if ((g_buffers->shapeFlags[i]&eNvFlexShapeFlagTypeMask) == eNvFlexShapeTriangleMesh)
			{
				numTriMeshInstances++;

				NvFlexCollisionGeometry geo = g_buffers->shapeGeometry[g_buffers->shapeStarts[i]];

				if (originalToNewMeshMap.find(geo.triMesh.mesh) == originalToNewMeshMap.end())
				{
					printf("Missing mesh for geometry entry\n");
					assert(0);
				}
				else
				{					
					g_buffers->shapeGeometry[g_buffers->shapeStarts[i]].mTriMesh.mMesh = originalToNewMeshMap[geo.triMesh.mesh];
				}
			}
		}

		printf("Num Tri Meshes: %d Num Tri Mesh Instances: %d\n", int(g_meshes.size()), numTriMeshInstances);

	}

	virtual void Draw(int pass)
	{
	}

	virtual void DoGui()
	{
	}

	virtual void Update()
	{

	}

	const char* mFilename;
	FILE* mRecording;

	std::vector<int> mInitialActive;
};
*/
