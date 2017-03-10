

class PlasticBody : public Scene
{
public:

	PlasticBody(const char* name, const char* mesh) :
		Scene(name),
		mFile(mesh),
		mScale(2.0f),
		mOffset(0.0f, 1.0f, 0.0f),
		mRadius(0.1f),
		mClusterSpacing(1.0f),
		mClusterRadius(0.0f),
		mClusterStiffness(0.5f),
		mLinkRadius(0.0f),
		mLinkStiffness(1.0f),
		mGlobalStiffness(1.0f),
		mSurfaceSampling(0.0f),
		mVolumeSampling(4.0f),
		mSkinningFalloff(2.0f),
		mSkinningMaxDistance(100.0f)
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
		g_params.dissipation = 0.0f;
		g_params.numIterations = 4;
		g_params.viscosity = 0.0f;
		g_params.drag = 0.0f;
		g_params.lift = 0.0f;
		g_params.collisionDistance = radius*0.75f;

		g_params.plasticThreshold = 0.0015f;
		g_params.plasticCreep = 0.125f;

		g_params.relaxationFactor = 0.6f;

		g_windStrength = 0.0f;

		g_numSubsteps = 2;

		// draw options
		g_drawPoints = false;
		g_wireframe = false;
		g_drawSprings = false;
		g_drawBases = false;

		g_buffers->rigidOffsets.push_back(0);

		mInstances.resize(0);


		CreateBodies();

		AddPlinth();

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
		// build soft body 
		for (int x = 0; x < mStack[0]; ++x)
		{
			for (int y = 0; y < mStack[1]; ++y)
			{
				for (int z = 0; z < mStack[2]; ++z)
				{
					CreatePlasticBody(mRadius, mOffset + Vec3(x*(mScale.x + 1), y*(mScale.y + 1), z*(mScale.z + 1))*mRadius, mClusterStiffness, mInstances.size());
				}
			}
		}
	}

	void CreatePlasticBody(float radius, Vec3 position, float clusterStiffness, int group = 0)
	{
		Instance instance;

		Mesh* mesh = ImportMesh(GetFilePathByPlatform(mFile).c_str());
		mesh->Normalize();
		mesh->Transform(TranslationMatrix(Point3(position))*ScaleMatrix(mScale*radius));

		instance.mMesh = mesh;
		instance.mColor = Vec3(0.5f, 0.5f, 1.0f);
		instance.mOffset = g_buffers->rigidTranslations.size();

		double createStart = GetSeconds();

		// create soft body definition
		NvFlexExtAsset* asset = NvFlexExtCreateSoftFromMesh(
			(float*)&instance.mMesh->m_positions[0],
			instance.mMesh->m_positions.size(),
			(int*)&instance.mMesh->m_indices[0],
			instance.mMesh->m_indices.size(),
			radius,
			mVolumeSampling,
			mSurfaceSampling,
			mClusterSpacing*radius,
			mClusterRadius*radius,
			clusterStiffness,
			mLinkRadius*radius,
			mLinkStiffness,
			mGlobalStiffness);

		double createEnd = GetSeconds();

		// create skinning
		const int maxWeights = 4;

		instance.mSkinningIndices.resize(instance.mMesh->m_positions.size()*maxWeights);
		instance.mSkinningWeights.resize(instance.mMesh->m_positions.size()*maxWeights);

		for (int i = 0; i < asset->numShapes; ++i)
			instance.mRigidRestPoses.push_back(Vec3(&asset->shapeCenters[i * 3]));

		double skinStart = GetSeconds();

		NvFlexExtCreateSoftMeshSkinning(
			(float*)&instance.mMesh->m_positions[0],
			instance.mMesh->m_positions.size(),
			asset->shapeCenters,
			asset->numShapes,
			mSkinningFalloff,
			mSkinningMaxDistance,
			&instance.mSkinningWeights[0],
			&instance.mSkinningIndices[0]);

		double skinEnd = GetSeconds();

		printf("Created soft in %f ms Skinned in %f\n", (createEnd - createStart)*1000.0f, (skinEnd - skinStart)*1000.0f);

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

		// add link data to the solver 
		for (int i = 0; i < asset->numSprings; ++i)
		{
			g_buffers->springIndices.push_back(asset->springIndices[i * 2 + 0]);
			g_buffers->springIndices.push_back(asset->springIndices[i * 2 + 1]);

			g_buffers->springStiffness.push_back(asset->springCoefficients[i]);
			g_buffers->springLengths.push_back(asset->springRestLengths[i]);
		}

		NvFlexExtDestroyAsset(asset);

		mInstances.push_back(instance);
	}

	virtual void Draw(int pass)
	{
		if (!g_drawMesh)
			return;

		for (int s = 0; s < int(mInstances.size()); ++s)
		{
			const Instance& instance = mInstances[s];

			Mesh m;
			m.m_positions.resize(instance.mMesh->m_positions.size());
			m.m_normals.resize(instance.mMesh->m_normals.size());
			m.m_indices = instance.mMesh->m_indices;

			for (int i = 0; i < int(instance.mMesh->m_positions.size()); ++i)
			{
				Vec3 softPos;
				Vec3 softNormal;

				for (int w = 0; w < 4; ++w)
				{
					const int cluster = instance.mSkinningIndices[i * 4 + w];
					const float weight = instance.mSkinningWeights[i * 4 + w];

					if (cluster > -1)
					{
						// offset in the global constraint array
						int rigidIndex = cluster + instance.mOffset;

						Vec3 localPos = Vec3(instance.mMesh->m_positions[i]) - instance.mRigidRestPoses[cluster];

						Vec3 skinnedPos = g_buffers->rigidTranslations[rigidIndex] + Rotate(g_buffers->rigidRotations[rigidIndex], localPos);
						Vec3 skinnedNormal = Rotate(g_buffers->rigidRotations[rigidIndex], instance.mMesh->m_normals[i]);

						softPos += skinnedPos*weight;
						softNormal += skinnedNormal*weight;
					}
				}

				m.m_positions[i] = Point3(softPos);
				m.m_normals[i] = softNormal;
			}

			DrawMesh(&m, instance.mColor);
		}
	}

	struct Instance
	{
		Mesh* mMesh;
		std::vector<int> mSkinningIndices;
		std::vector<float> mSkinningWeights;
		vector<Vec3> mRigidRestPoses;
		Vec3 mColor;
		int mOffset;
	};

	std::vector<Instance> mInstances;

	const char* mFile;
	Vec3 mScale;
	Vec3 mOffset;

	float mRadius;

	float mClusterSpacing;
	float mClusterRadius;
	float mClusterStiffness;

	float mLinkRadius;
	float mLinkStiffness;

	float mGlobalStiffness;

	float mSurfaceSampling;
	float mVolumeSampling;

	float mSkinningFalloff;
	float mSkinningMaxDistance;

	int mStack[3];
};