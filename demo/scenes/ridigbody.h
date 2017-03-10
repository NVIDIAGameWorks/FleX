class RigidBody : public Scene
{
public:

	RigidBody(const char* name, const char* mesh) :
		Scene(name),
		mFile(mesh),
		mScale(2.0f),
		mOffset(0.0f, 1.0f, 0.0f),
		mRadius(0.1f)
	{
		mStack[0] = 1;
		mStack[1] = 1;
		mStack[2] = 1;
	}

	virtual void Initialize()
	{
		float radius = mRadius;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.35f;
		g_params.particleFriction = 0.25f;
		g_params.numIterations = 4;
		g_params.collisionDistance = radius*0.75f;

		g_numSubsteps = 2;

		// draw options
		g_drawPoints = true;
		g_wireframe = false;
		g_drawSprings = false;
		g_drawBases = true;

		g_buffers->rigidOffsets.push_back(0);

		mInstances.resize(0);


		CreateBodies();

		// fix any particles below the ground plane in place
		for (int i = 0; i < int(g_buffers->positions.size()); ++i)
			if (g_buffers->positions[i].y < 0.0f)
				g_buffers->positions[i].w = 0.0f;

		// expand radius for better self collision
		g_params.radius *= 1.5f;

		g_lightDistance *= 1.5f;
	}

	virtual void CreateBodies()
	{
		// build hard body 
		for (int x = 0; x < mStack[0]; ++x)
		{
			for (int y = 0; y < mStack[1]; ++y)
			{
				for (int z = 0; z < mStack[2]; ++z)
				{
					CreateRigidBody(mRadius, mOffset + Vec3(x*(mScale.x + 1), y*(mScale.y + 1), z*(mScale.z + 1))*mRadius);
				}
			}
		}
	}

	void CreateRigidBody(float radius, Vec3 position, int group = 0)
	{
		Instance instance;

		Mesh* mesh = ImportMesh(GetFilePathByPlatform(mFile).c_str());
		mesh->Normalize();
		mesh->Transform(TranslationMatrix(Point3(position))*ScaleMatrix(mScale*radius));

		instance.mMesh = mesh;
		instance.mColor = Vec3(0.5f, 0.5f, 1.0f);

		double createStart = GetSeconds();

		//const float spacing = radius;
		const float spacing = radius*0.5f;

		NvFlexExtAsset* asset = NvFlexExtCreateRigidFromMesh(
			(float*)&mesh->m_positions[0],
			int(mesh->m_positions.size()),
			(int*)&mesh->m_indices[0],
			mesh->m_indices.size(),
			spacing,
			-spacing*0.5f);

		double createEnd = GetSeconds();

		const int particleOffset = g_buffers->positions.size();
		const int indexOffset = g_buffers->rigidOffsets.back();

		// add particle data to solver
		for (int i = 0; i < asset->numParticles; ++i)
		{
			g_buffers->positions.push_back(&asset->particles[i * 4]);
			g_buffers->velocities.push_back(0.0f);

			const int phase = NvFlexMakePhase(group, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);
			g_buffers->phases.push_back(phase);
		}

		// add shape data to solver
		for (int i = 0; i < asset->numShapeIndices; ++i)
			g_buffers->rigidIndices.push_back(asset->shapeIndices[i] + particleOffset);

		for (int i = 0; i < asset->numShapes; ++i)
		{
			g_buffers->rigidOffsets.push_back(asset->shapeOffsets[i] + indexOffset);
			g_buffers->rigidTranslations.push_back(Vec3(&asset->shapeCenters[i * 3]));
			g_buffers->rigidRotations.push_back(Quat());
			g_buffers->rigidCoefficients.push_back(asset->shapeCoefficients[i]);
		}

		NvFlexExtDestroyAsset(asset);

		mInstances.push_back(instance);
	}

	struct Instance
	{
		Mesh* mMesh;
		Vec3 mColor;
	};

	std::vector<Instance> mInstances;

	const char* mFile;
	Vec3 mScale;
	Vec3 mOffset;

	float mRadius;

	int mStack[3];
};