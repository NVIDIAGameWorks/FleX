


class GranularShape : public Scene
{
public:

	GranularShape(const char* name) : Scene(name) {}

	void Initialize()
	{
		// granular dragon
		CreateParticleShape(GetFilePathByPlatform("../../data/dragon.obj").c_str(),Vec3(0.0f, 2.5f, 0.0f), 16.0f, DegToRad(-20.0f), g_params.radius*1.05f, Vec3(0.0f, 0.0f, 0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), false, g_params.radius*0.05f);
		
		AddBox(Vec3(8.0f, 8.0f, 5.0f));
		g_buffers->shapePositions[0] += Vec4(0.0f, -1.5f, 0.0f, 0.0f);
		
		g_params.staticFriction = 1.0f;
		g_params.dynamicFriction = 0.65f;
		g_params.dissipation = 0.01f;
		g_params.numIterations = 6;
		g_params.particleCollisionMargin = g_params.radius*0.5f;	// 5% collision margin
		g_params.sleepThreshold = g_params.radius*0.35f;
		g_params.shockPropagation = 3.f;
		g_params.restitution = 0.01f;
		g_params.gravity[1] *= 1.f;

		g_numSubsteps = 3;

		extern Colour gColors[];
		gColors[1] = Colour(0.805f, 0.702f, 0.401f);		

		// draw options		
		g_drawPoints = true;
	}
};
