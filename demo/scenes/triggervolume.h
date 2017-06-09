


class TriggerVolume : public Scene
{
public:

	TriggerVolume(const char* name) : Scene(name) {}

	void Initialize()
	{
		float radius = 0.05f;
		CreateParticleGrid(Vec3(1.75, 2.0, -0.25), 10, 5, 10, radius, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), 0.0f);

		g_numExtraParticles = 10000;

		// regular box
		AddBox(Vec3(0.5), Vec3(0.0, 0.5, 0.0));

		// trigger box
		AddBox(Vec3(0.5), Vec3(2.0f, 0.5, 0.0));
		g_buffers->shapeFlags[1] |= eNvFlexShapeFlagTrigger;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.025f;
		g_params.dissipation = 0.0f;
		g_params.restitution = 0.0;
		g_params.numIterations = 4;
		g_params.particleCollisionMargin = g_params.radius*0.05f;

		g_numSubsteps = 1;

		// draw options		
		g_drawPoints = true;

		g_emitters[0].mEnabled = true;
	}

	virtual void Update()
	{
		const int maxContactsPerParticle = 6;

		NvFlexVector<Vec4> contactPlanes(g_flexLib,g_buffers->positions.size()*maxContactsPerParticle);
		NvFlexVector<Vec4> contactVelocities(g_flexLib, g_buffers->positions.size()*maxContactsPerParticle);
		NvFlexVector<int> contactIndices(g_flexLib, g_buffers->positions.size());
		NvFlexVector<unsigned int> contactCounts(g_flexLib, g_buffers->positions.size());

		NvFlexGetContacts(g_solver, contactPlanes.buffer, contactVelocities.buffer, contactIndices.buffer, contactCounts.buffer);

		contactPlanes.map();
		contactVelocities.map();
		contactIndices.map();
		contactCounts.map();

		int activeCount = NvFlexGetActiveCount(g_solver);

		for (int i = 0; i < activeCount; ++i)
		{
			const int contactIndex = contactIndices[i];
			const unsigned int count = contactCounts[contactIndex];

			for (unsigned int c = 0; c < count; ++c)
			{
				Vec4 velocity = contactVelocities[contactIndex*maxContactsPerParticle + c];

				const int shapeId = int(velocity.w);

				// detect when particle intersects the trigger 
				// volume and teleport it over to the other box
				if (shapeId == 1)
				{
					Vec3 pos = Vec3(Randf(-0.5f, 0.5f), 1.0f, Randf(-0.5f, 0.5f));

					g_buffers->positions[i] = Vec4(pos, 1.0f);
					g_buffers->velocities[i] = 0.0f;
				}
			}
		}
	}

};