

class Inflatable : public Scene
{
public:

	Inflatable(const char* name) : Scene(name) {}

	virtual ~Inflatable()
	{
		for (size_t i = 0; i < mCloths.size(); ++i)
			delete mCloths[i];
	}

	void AddInflatable(const Mesh* mesh, float overPressure, int phase)
	{
		const int startVertex = g_buffers->positions.size();

		// add mesh to system
		for (size_t i = 0; i < mesh->GetNumVertices(); ++i)
		{
			const Vec3 p = Vec3(mesh->m_positions[i]);

			g_buffers->positions.push_back(Vec4(p.x, p.y, p.z, 1.0f));
			g_buffers->velocities.push_back(0.0f);
			g_buffers->phases.push_back(phase);
		}

		int triOffset = g_buffers->triangles.size();
		int triCount = mesh->GetNumFaces();

		g_buffers->inflatableTriOffsets.push_back(triOffset / 3);
		g_buffers->inflatableTriCounts.push_back(mesh->GetNumFaces());
		g_buffers->inflatablePressures.push_back(overPressure);

		for (size_t i = 0; i < mesh->m_indices.size(); i += 3)
		{
			int a = mesh->m_indices[i + 0];
			int b = mesh->m_indices[i + 1];
			int c = mesh->m_indices[i + 2];

			Vec3 n = -Normalize(Cross(mesh->m_positions[b] - mesh->m_positions[a], mesh->m_positions[c] - mesh->m_positions[a]));
			g_buffers->triangleNormals.push_back(n);

			g_buffers->triangles.push_back(a + startVertex);
			g_buffers->triangles.push_back(b + startVertex);
			g_buffers->triangles.push_back(c + startVertex);
		}

		// create a cloth mesh using the global positions / indices
		ClothMesh* cloth = new ClothMesh(&g_buffers->positions[0], g_buffers->positions.size(), &g_buffers->triangles[triOffset], triCount * 3, 0.8f, 1.0f);

		for (size_t i = 0; i < cloth->mConstraintIndices.size(); ++i)
			g_buffers->springIndices.push_back(cloth->mConstraintIndices[i]);

		for (size_t i = 0; i < cloth->mConstraintCoefficients.size(); ++i)
			g_buffers->springStiffness.push_back(cloth->mConstraintCoefficients[i]);

		for (size_t i = 0; i < cloth->mConstraintRestLengths.size(); ++i)
			g_buffers->springLengths.push_back(cloth->mConstraintRestLengths[i]);

		mCloths.push_back(cloth);

		// add inflatable params
		g_buffers->inflatableVolumes.push_back(cloth->mRestVolume);
		g_buffers->inflatableCoefficients.push_back(cloth->mConstraintScale);
	}

	void Initialize()
	{
		mCloths.resize(0);

		float minSize = 0.75f;
		float maxSize = 1.0f;

		// convex rocks
		for (int i = 0; i < 5; i++)
			AddRandomConvex(10, Vec3(i*2.0f, 0.0f, Randf(0.0f, 2.0f)), minSize, maxSize, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi));

		float radius = 0.12f;
		int group = 0;

		const char* meshes[2] =
		{
			"../../data/box_high_weld.ply",
			"../../data/sphere.ply"
		};

		mPressure = 1.0f;

		for (int y = 0; y < 2; ++y)
		{
			for (int i = 0; i < 4; ++i)
			{
				Mesh* mesh = ImportMesh(GetFilePathByPlatform(meshes[(i + y) & 1]).c_str());
				mesh->Normalize();
				mesh->Transform(TranslationMatrix(Point3(i*2.0f, 1.0f + y*2.0f, 1.5f)));

				AddInflatable(mesh, mPressure, NvFlexMakePhase(group++, 0));

				delete mesh;
			}
		}

		g_params.radius = radius;
		g_params.dynamicFriction = 0.4f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 10;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.drag = 0.0f;
		g_params.collisionDistance = 0.01f;

		// better convergence with global relaxation factor
		g_params.relaxationMode = eNvFlexRelaxationGlobal;
		g_params.relaxationFactor = 0.25f;

		g_windStrength = 0.0f;

		g_numSubsteps = 2;

		// draw options		
		g_drawPoints = false;
		g_drawSprings = 0;
		g_drawCloth = false;
	}

	virtual void DoGui()
	{
		if (imguiSlider("Over Pressure", &mPressure, 0.25f, 3.0f, 0.001f))
		{
			for (int i = 0; i < int(g_buffers->inflatablePressures.size()); ++i)
				g_buffers->inflatablePressures[i] = mPressure;
		}
	}

	virtual void Sync()
	{
		NvFlexSetInflatables(g_flex, g_buffers->inflatableTriOffsets.buffer, g_buffers->inflatableTriCounts.buffer, g_buffers->inflatableVolumes.buffer, g_buffers->inflatablePressures.buffer, g_buffers->inflatableCoefficients.buffer, mCloths.size());
	}

	virtual void Draw(int pass)
	{
		if (!g_drawMesh)
			return;

		int indexStart = 0;

		for (size_t i = 0; i < mCloths.size(); ++i)
		{
			DrawCloth(&g_buffers->positions[0], &g_buffers->normals[0], NULL, &g_buffers->triangles[indexStart], mCloths[i]->mTris.size(), g_buffers->positions.size(), i % 6, g_params.radius*0.35f);

			indexStart += mCloths[i]->mTris.size() * 3;
		}
	}

	float mPressure;

	std::vector<ClothMesh*> mCloths;
};
