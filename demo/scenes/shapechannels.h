

// demonstrates how to make particles collide with 
// selected collision shapes based on the channel mask
class ShapeChannels : public Scene
{
public:

	ShapeChannels(const char* name) : Scene(name) {}

	void Initialize()
	{
		const float radius = 0.05f;
		
		Vec3 offset = Vec3(-0.25f, 6.0f, -0.25f);

		// layers of particles each set to collide with a different shape channel
		int numLayers = 8;

		for (int i=0; i < numLayers; ++i)
		{
			CreateParticleGrid(offset + Vec3(0.0f, 2.0f*i*radius, 0.0f), 10, 1, 10, radius, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhaseWithChannels(i, eNvFlexPhaseSelfCollide, eNvFlexPhaseShapeChannel0<<i), 0.0f);
			//CreateParticleGrid(offset + Vec3(0.0f, 2.0f*radius, 0.0f), 10, 1, 10, radius, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhaseWithChannels(0, eNvFlexPhaseSelfCollide, eNvFlexPhaseShapeChannel1), 0.0f);
			//CreateParticleGrid(offset + Vec3(0.0f, 4.0f*radius, 0.0f), 10, 1, 10, radius, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhaseWithChannels(0, eNvFlexPhaseSelfCollide, eNvFlexPhaseShapeChannel2), 0.0f);
		
			AddBox(Vec3(0.5f, 0.1f, 0.5f), Vec3(0.0f, 0.5f + i*0.5f, 0.0f), Quat(), false, eNvFlexPhaseShapeChannel0<<i);
		}

		// regular box
		//AddBox(Vec3(0.5f, 0.1f, 0.5f), Vec3(0.0f, 0.5f, 0.0f), Quat(), false, eNvFlexPhaseShapeChannel0);
		//AddBox(Vec3(0.5f, 0.1f, 0.5f), Vec3(0.0f, 1.0f, 0.0f), Quat(), false, eNvFlexPhaseShapeChannel1);
		//AddBox(Vec3(0.5f, 0.1f, 0.5f), Vec3(0.0f, 1.5f, 0.0f), Quat(), false, eNvFlexPhaseShapeChannel2);

		g_params.radius = radius;
		g_params.dynamicFriction = 0.025f;
		g_params.dissipation = 0.0f;
		g_params.restitution = 0.0;
		g_params.numIterations = 4;
		g_params.particleCollisionMargin = g_params.radius*0.05f;

		g_numSubsteps = 1;
	}

	virtual void Update()
	{
		
	}
};