

class ThinBox : public Scene
{
public:

	ThinBox(const char* name) : Scene(name) {}

	int base;
	int width;
	int height;
	int depth;

	virtual void Initialize()
	{
		float radius = 0.03f;

		width = 16;
		height = 8;
		depth = 8;

		base = 4;

		float sep = radius;

		CreateParticleGrid(Vec3(0.0f, radius, 0.0f), width, height, depth, sep, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), 0.0f);

		Vec3 upper;
		Vec3 lower;
		GetParticleBounds(lower, upper);
		lower -= Vec3(radius*0.5f);
		upper += Vec3(radius*0.5f);

		Vec3 center = 0.5f*(upper + lower);

		float width = (upper - lower).x*0.5f;
		float depth = (upper - lower).z*0.5f;
		float edge = 0.0075f*0.5f;
		float height = 8 * radius;

		AddBox(Vec3(edge, height, depth), center + Vec3(-width, height / 2, 0.0f));
		AddBox(Vec3(edge, height, depth), center + Vec3(width, height / 2, 0.0f));
		AddBox(Vec3(width - edge, height, edge), center + Vec3(0.0f, height / 2, (depth - edge)));
		AddBox(Vec3(width - edge, height, edge), center + Vec3(0.0f, height / 2, -(depth - edge)));
		AddBox(Vec3(width, edge, depth), Vec3(center.x, lower.y, center.z));

		g_params.gravity[1] = -9.f;
		g_params.radius = radius;
		g_params.dynamicFriction = 0.0f;
		g_params.fluid = false;
		g_params.numIterations = 5;
		g_params.numPlanes = 1;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = radius;
		g_params.particleCollisionMargin = radius*0.5f;

		g_params.relaxationFactor = 0.0f;
		g_numSubsteps = 2;

		g_lightDistance *= 0.85f;

		// draw options
		g_drawPoints = true;
		g_warmup = false;
	}
};