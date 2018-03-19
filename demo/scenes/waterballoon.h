
class WaterBalloon : public Scene
{
public:

	WaterBalloon(const char* name) : Scene(name) {}

	virtual ~WaterBalloon()
	{
		for (size_t i = 0; i < mCloths.size(); ++i)
			NvFlexExtDestroyTearingCloth(mCloths[i].asset);
	}

	void AddInflatable(const Mesh* mesh, float overPressure, float invMass, int phase)
	{
		// create a cloth mesh using the global positions / indices
		const int numParticles = int(mesh->m_positions.size());
		const int maxParticles = numParticles * 2;

		Balloon balloon;
		balloon.particleOffset = g_buffers->positions.size();
		balloon.triangleOffset = g_buffers->triangles.size();
		balloon.splitThreshold = 4.0f;

		// add particles to system
		for (size_t i = 0; i < mesh->GetNumVertices(); ++i)
		{
			const Vec3 p = Vec3(mesh->m_positions[i]);

			g_buffers->positions.push_back(Vec4(p.x, p.y, p.z, invMass));
			g_buffers->restPositions.push_back(Vec4(p.x, p.y, p.z, invMass));

			g_buffers->velocities.push_back(0.0f);
			g_buffers->phases.push_back(phase);
		}

		for (size_t i = 0; i < mesh->m_indices.size(); i += 3)
		{
			int a = mesh->m_indices[i + 0];
			int b = mesh->m_indices[i + 1];
			int c = mesh->m_indices[i + 2];

			Vec3 n = -Normalize(Cross(mesh->m_positions[b] - mesh->m_positions[a], mesh->m_positions[c] - mesh->m_positions[a]));
			g_buffers->triangleNormals.push_back(n);

			g_buffers->triangles.push_back(a + balloon.particleOffset);
			g_buffers->triangles.push_back(b + balloon.particleOffset);
			g_buffers->triangles.push_back(c + balloon.particleOffset);
		}

		// create tearing asset
		NvFlexExtAsset* cloth = NvFlexExtCreateTearingClothFromMesh((float*)&g_buffers->positions[balloon.particleOffset], numParticles, maxParticles, (int*)&mesh->m_indices[0], mesh->GetNumFaces(), 1.0f, 1.0f, 0.0f);
		balloon.asset = cloth;

		mCloths.push_back(balloon);
	}

	void Initialize()
	{
		mCloths.resize(0);

		float minSize = 0.25f;
		float maxSize = 0.5f;
		float spacing = 4.0f;

		// convex rocks
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 1; j++)
				AddRandomConvex(10, Vec3(i*maxSize*spacing, 0.0f, j*maxSize*spacing), minSize, maxSize, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi));

		float radius = 0.1f;
		int group = 0;

		g_numExtraParticles = 20000;
		g_numSubsteps = 3;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.125f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 5;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.relaxationFactor = 1.0f;
		g_params.drag = 0.0f;
		g_params.smoothing = 1.f;
		g_params.maxSpeed = 0.5f*g_numSubsteps*radius / g_dt;
		g_params.gravity[1] *= 1.0f;
		g_params.collisionDistance = 0.01f;
		g_params.solidPressure = 0.0f;

		g_params.fluidRestDistance = radius*0.65f;
		g_params.viscosity = 0.0;
		g_params.adhesion = 0.0f;
		g_params.cohesion = 0.02f;


		// add inflatables
		Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/sphere_high.ply").c_str());

		for (int y = 0; y < 2; ++y)
			for (int i = 0; i < 2; ++i)
			{
				Vec3 lower = Vec3(2.0f + i*2.0f, 0.4f + y*1.2f, 1.0f);

				mesh->Normalize();
				mesh->Transform(TranslationMatrix(Point3(lower)));

				AddInflatable(mesh, 1.0f, 0.25f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter));
			}

		g_numSolidParticles = g_buffers->positions.size();
		g_numExtraParticles = g_buffers->positions.size();

		// fill inflatables with water
		std::vector<Vec3> positions(10000);
		int n = PoissonSample3D(0.45f, g_params.radius*0.42f, &positions[0], positions.size(), 10000);
		//int n = TightPack3D(0.45f, g_params.radius*0.42f, &positions[0], positions.size());

		mNumFluidParticles = 0;

		for (size_t i = 0; i < mCloths.size(); ++i)
		{
			const int vertStart = i*mesh->GetNumVertices();
			const int vertEnd = vertStart + mesh->GetNumVertices();

			const int phase = NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);

			Vec3 center;
			for (int v = vertStart; v < vertEnd; ++v)
				center += Vec3(g_buffers->positions[v]);

			center /= float(vertEnd - vertStart);

			printf("%d, %d - %f %f %f\n", vertStart, vertEnd, center.x, center.y, center.z);

			for (int i = 0; i < n; ++i)
			{
				g_buffers->positions.push_back(Vec4(center + positions[i], 1.0f));
				g_buffers->restPositions.push_back(Vec4());
				g_buffers->velocities.push_back(0.0f);
				g_buffers->phases.push_back(phase);
			}

			mNumFluidParticles += n;
		}

		delete mesh;

		g_drawPoints = false;
		g_drawEllipsoids = true;
		g_drawSprings = 0;
		g_drawCloth = false;
		g_warmup = true;

	}

	void RebuildConstraints()
	{
		// update constraint data
		g_buffers->triangles.resize(0);
		g_buffers->springIndices.resize(0);
		g_buffers->springStiffness.resize(0);
		g_buffers->springLengths.resize(0);

		for (int c = 0; c < int(mCloths.size()); ++c)
		{
			Balloon& balloon = mCloths[c];

			for (int i = 0; i < balloon.asset->numTriangles; ++i)
			{
				g_buffers->triangles.push_back(balloon.asset->triangleIndices[i * 3 + 0] + balloon.particleOffset);
				g_buffers->triangles.push_back(balloon.asset->triangleIndices[i * 3 + 1] + balloon.particleOffset);
				g_buffers->triangles.push_back(balloon.asset->triangleIndices[i * 3 + 2] + balloon.particleOffset);
			}

			for (int i = 0; i < balloon.asset->numSprings * 2; ++i)
				g_buffers->springIndices.push_back(balloon.asset->springIndices[i] + balloon.particleOffset);


			for (int i = 0; i < balloon.asset->numSprings; ++i)
			{
				g_buffers->springStiffness.push_back(balloon.asset->springCoefficients[i]);
				g_buffers->springLengths.push_back(balloon.asset->springRestLengths[i]);
			}
		}
	}

	virtual void Sync()
	{
		// send new particle data to the GPU
		NvFlexSetRestParticles(g_solver, g_buffers->restPositions.buffer, NULL);

		// update solver
		NvFlexSetSprings(g_solver, g_buffers->springIndices.buffer, g_buffers->springLengths.buffer, g_buffers->springStiffness.buffer, g_buffers->springLengths.size());
		NvFlexSetDynamicTriangles(g_solver, g_buffers->triangles.buffer, g_buffers->triangleNormals.buffer, g_buffers->triangles.size() / 3);
	}

	virtual void Update()
	{
		// temporarily restore the mouse particle's mass so that we can tear it
		if (g_mouseParticle != -1)
			g_buffers->positions[g_mouseParticle].w = g_mouseMass;

		// force larger radius for solid interactions to prevent interpenetration
		g_params.solidRestDistance = g_params.radius;

		// build new particle arrays
		std::vector<Vec4> newParticles;
		std::vector<Vec4> newParticlesRest;
		std::vector<Vec3> newVelocities;
		std::vector<int> newPhases;
		std::vector<Vec4> newNormals;

		for (int c = 0; c < int(mCloths.size()); ++c)
		{
			Balloon& balloon = mCloths[c];

			const int destOffset = newParticles.size();

			// append existing particles
			for (int i = 0; i < balloon.asset->numParticles; ++i)
			{
				newParticles.push_back(g_buffers->positions[balloon.particleOffset + i]);
				newParticlesRest.push_back(g_buffers->restPositions[balloon.particleOffset + i]);
				newVelocities.push_back(g_buffers->velocities[balloon.particleOffset + i]);
				newPhases.push_back(g_buffers->phases[balloon.particleOffset + i]);
				newNormals.push_back(g_buffers->normals[balloon.particleOffset + i]);
			}

			// perform splitting
			const int maxCopies = 2048;
			const int maxEdits = 2048;

			NvFlexExtTearingParticleClone particleCopies[maxCopies];
			int numParticleCopies;

			NvFlexExtTearingMeshEdit triangleEdits[maxEdits];
			int numTriangleEdits;

			// update asset's copy of the particles
			memcpy(balloon.asset->particles, &g_buffers->positions[balloon.particleOffset], sizeof(Vec4)*balloon.asset->numParticles);

			// tear 
			NvFlexExtTearClothMesh(balloon.asset, balloon.splitThreshold, 1, particleCopies, &numParticleCopies, maxCopies, triangleEdits, &numTriangleEdits, maxEdits);

			// resize particle data arrays
			newParticles.resize(newParticles.size() + numParticleCopies);
			newParticlesRest.resize(newParticlesRest.size() + numParticleCopies);
			newVelocities.resize(newVelocities.size() + numParticleCopies);
			newPhases.resize(newPhases.size() + numParticleCopies);
			newNormals.resize(newNormals.size() + numParticleCopies);

			// copy particles
			for (int i = 0; i < numParticleCopies; ++i)
			{
				const int srcIndex = balloon.particleOffset + particleCopies[i].srcIndex;
				const int destIndex = destOffset + particleCopies[i].destIndex;

				newParticles[destIndex] = g_buffers->positions[srcIndex];
				newParticlesRest[destIndex] = g_buffers->restPositions[srcIndex];
				newVelocities[destIndex] = g_buffers->velocities[srcIndex];
				newPhases[destIndex] = g_buffers->phases[srcIndex];
				newNormals[destIndex]  = g_buffers->normals[srcIndex];
			}

			if (numParticleCopies)
			{
				// reduce split threshold for this balloon
				balloon.splitThreshold = 1.75f;
			}

			balloon.particleOffset = destOffset;
			balloon.asset->numParticles += numParticleCopies;
		}

		// append fluid particles
		const int fluidStart = g_numSolidParticles;
		const int fluidEnd = fluidStart + mNumFluidParticles;

		g_numSolidParticles = newParticles.size();

		for (int i = fluidStart; i < fluidEnd; ++i)
		{
			newParticles.push_back(g_buffers->positions[i]);
			newParticlesRest.push_back(Vec4());
			newVelocities.push_back(g_buffers->velocities[i]);
			newPhases.push_back(g_buffers->phases[i]);
			newNormals.push_back(g_buffers->normals[i]);
		}

		g_buffers->positions.assign(&newParticles[0], newParticles.size());
		g_buffers->restPositions.assign(&newParticlesRest[0], newParticlesRest.size());
		g_buffers->velocities.assign(&newVelocities[0], newVelocities.size());
		g_buffers->phases.assign(&newPhases[0], newPhases.size());
		g_buffers->normals.assign(&newNormals[0], newNormals.size());

		// build active indices list
		g_buffers->activeIndices.resize(g_buffers->positions.size());
		for (int i = 0; i < g_buffers->positions.size(); ++i)
			g_buffers->activeIndices[i] = i;

		// update constraint buffers
		RebuildConstraints();

		// restore mouse mass		
		if (g_mouseParticle != -1)
			g_buffers->positions[g_mouseParticle].w = 0.0f;
	}

	virtual void Draw(int pass)
	{
		if (!g_drawMesh)
			return;

		for (size_t i = 0; i < mCloths.size(); ++i)
		{
			DrawCloth(&g_buffers->positions[0], &g_buffers->normals[0], NULL, &g_buffers->triangles[mCloths[i].triangleOffset], mCloths[i].asset->numTriangles, g_buffers->positions.size(), (i + 2) % 6);//, g_params.radius*0.25f);			
		}
	}

	struct Balloon
	{
		NvFlexExtAsset* asset;

		int particleOffset;
		int triangleOffset;

		float splitThreshold;
	};

	int mNumFluidParticles;

	std::vector<Balloon> mCloths;
};
