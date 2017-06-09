
class Tearing : public Scene
{
public:

	Tearing(const char* name) : Scene(name) {}

	~Tearing()
	{
		NvFlexExtDestroyTearingCloth(mCloth);
	}

	void Initialize()
	{
		Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/irregular_plane.obj").c_str());
		mesh->Transform(RotationMatrix(kPi, Vec3(0.0f, 1.0f, 0.0f))*RotationMatrix(kPi*0.5f, Vec3(1.0f, 0.0f, 0.0f))*ScaleMatrix(2.0f));

		Vec3 lower, upper;
		mesh->GetBounds(lower, upper);

		float radius = 0.065f;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);

		for (size_t i = 0; i < mesh->GetNumVertices(); ++i)
		{
			Vec3 p = Vec3(mesh->m_positions[i]);

			float invMass = 1.0f;

			if (p.y == upper.y)
				invMass = 0.0f;

			p += Vec3(0.0f, 1.5f, 0.0f);

			g_buffers->positions.push_back(Vec4(p.x, p.y, p.z, invMass));
			g_buffers->velocities.push_back(0.0f);
			g_buffers->phases.push_back(phase);
		}

		g_numExtraParticles = 1000;

		mCloth = NvFlexExtCreateTearingClothFromMesh((float*)&g_buffers->positions[0], int(g_buffers->positions.size()), int(g_buffers->positions.size()) + g_numExtraParticles, (int*)&mesh->m_indices[0], mesh->GetNumFaces(), 0.8f, 0.8f, 0.0f);

		g_buffers->triangles.assign((int*)&mesh->m_indices[0], mesh->m_indices.size());
		g_buffers->triangleNormals.resize(mesh->GetNumFaces(), Vec3(0.0f, 0.0f, 1.0f));

		g_buffers->springIndices.assign(mCloth->springIndices, mCloth->numSprings * 2);
		g_buffers->springStiffness.assign(mCloth->springCoefficients, mCloth->numSprings);
		g_buffers->springLengths.assign(mCloth->springRestLengths, mCloth->numSprings);

		g_params.radius = radius;
		g_params.dynamicFriction = 0.025f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 16;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.relaxationFactor = 1.0f;
		g_params.drag = 0.03f;

		g_params.relaxationMode = eNvFlexRelaxationGlobal;
		g_params.relaxationFactor = 0.35f;

		g_numSubsteps = 2;

		g_pause = false;

		// draw options		
		g_drawPoints = false;
	}

	void Update()
	{
		g_params.wind[0] = 0.1f;
		g_params.wind[1] = 0.1f;
		g_params.wind[2] = -0.2f;
		g_windStrength = 6.0f;

		const float maxStrain = 3.0f;
		const int maxCopies = 2048;
		const int maxEdits = 2048;

		NvFlexExtTearingParticleClone particleCopies[maxCopies];
		int numParticleCopies;

		NvFlexExtTearingMeshEdit triangleEdits[maxEdits];
		int numTriangleEdits;

		// update asset's copy of the particles
		memcpy(mCloth->particles, &g_buffers->positions[0], sizeof(Vec4)*g_buffers->positions.size());

		NvFlexExtTearClothMesh(mCloth, maxStrain, 4, particleCopies, &numParticleCopies, maxCopies, triangleEdits, &numTriangleEdits, maxEdits);

		// copy particles
		for (int i = 0; i < numParticleCopies; ++i)
		{
			const int srcIndex = particleCopies[i].srcIndex;
			const int destIndex = particleCopies[i].destIndex;

			g_buffers->positions[destIndex] = Vec4(Vec3(g_buffers->positions[srcIndex]), 1.0f);	// override mass because picked particle has inf. mass
			g_buffers->restPositions[destIndex] = g_buffers->restPositions[srcIndex];
			g_buffers->velocities[destIndex] = g_buffers->velocities[srcIndex];
			g_buffers->phases[destIndex] = g_buffers->phases[srcIndex];

			g_buffers->activeIndices.push_back(destIndex);
		}

		// apply triangle modifications to index buffer
		for (int i = 0; i < numTriangleEdits; ++i)
		{
			const int index = triangleEdits[i].triIndex;
			const int newValue = triangleEdits[i].newParticleIndex;

			g_buffers->triangles[index] = newValue;
		}

		mCloth->numParticles += numParticleCopies;

		// update constraints
		g_buffers->springIndices.assign(mCloth->springIndices, mCloth->numSprings * 2);
		g_buffers->springStiffness.assign(mCloth->springCoefficients, mCloth->numSprings);
		g_buffers->springLengths.assign(mCloth->springRestLengths, mCloth->numSprings);
	}

	virtual void Sync()
	{
		// update solver data not already updated in the main loop
		NvFlexSetSprings(g_solver, g_buffers->springIndices.buffer, g_buffers->springLengths.buffer, g_buffers->springStiffness.buffer, g_buffers->springLengths.size());
		NvFlexSetDynamicTriangles(g_solver, g_buffers->triangles.buffer, g_buffers->triangleNormals.buffer, g_buffers->triangles.size() / 3);
		NvFlexSetRestParticles(g_solver, g_buffers->restPositions.buffer, NULL);
	}

	NvFlexExtAsset* mCloth;
};