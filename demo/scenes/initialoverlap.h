

// tests initial particle overlap, particle should be projected out of the box without high velocity
class InitialOverlap : public Scene
{
public:

	InitialOverlap(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		g_params.radius = 0.1f;
		g_params.numIterations = 2;

		// test max acceleration clamping is working, test at 5x gravity
		g_params.maxAcceleration = 50.0f;

		// plinth
		AddBox(1.0f, Vec3(0.0f, 0.0f, 0.0f));

		g_buffers->positions.push_back(Vec4(0.0f, 0.5f, 0.0f, 1.0f));
		g_buffers->velocities.push_back(Vec3(0.0f));
		g_buffers->phases.push_back(0);

		g_numSubsteps = 2;

		// draw options		
		g_drawPoints = true;
	}
};
