

class ShapeCollision : public Scene
{
public:

	ShapeCollision(const char* name) : Scene(name) {}

	void Initialize()
	{
		float maxShapeRadius = 0.25f;
		float minShapeRadius = 0.1f;

		int dimx = 4;
		int dimy = 4;
		int dimz = 4;

		float radius = 0.05f;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);

		g_numSubsteps = 2;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.25f;
		g_params.dissipation = 0.0f;
		g_params.restitution = 0.0;
		g_params.numIterations = 2;
		g_params.particleCollisionMargin = g_params.radius*0.5f;
		g_params.shapeCollisionMargin = g_params.radius*0.5f;
		g_params.maxSpeed = 0.5f*radius*float(g_numSubsteps)/g_dt;
						
		CreateParticleGrid(Vec3(0.0f, 1.0f + dimy*maxShapeRadius*2.0f, 0.0f), 30, 50, 30, radius, Vec3(0.0f), 1.0f, false, 0.0f, phase, 0.0f);

		Mesh* box = ImportMesh(GetFilePathByPlatform("../../data/box.ply").c_str());
		box->Normalize(1.0f);

		NvFlexTriangleMeshId mesh = CreateTriangleMesh(box);
		delete box;

		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/bunny.ply").c_str(), 128);

		for (int i=0; i < dimx; ++i)
		{
			for (int j=0; j < dimy; ++j)
			{
				for (int k=0; k < dimz; ++k)
				{
					int type = Rand()%6;

					Vec3 shapeTranslation = Vec3(float(i),float(j) + 0.5f,float(k))*maxShapeRadius*2.0f;
					Quat shapeRotation = QuatFromAxisAngle(UniformSampleSphere(), Randf()*k2Pi);

					switch(type)
					{
						case 0:
						{
							AddSphere(Randf(minShapeRadius, maxShapeRadius), shapeTranslation, shapeRotation);
							break;
						}
						case 1:
						{
							AddCapsule(Randf(minShapeRadius, maxShapeRadius)*0.5f, Randf()*maxShapeRadius, shapeTranslation, shapeRotation);
							break;
						}
						case 2:
						{
							Vec3 extents = 0.75f*Vec3(Randf(minShapeRadius, maxShapeRadius),
												      Randf(minShapeRadius, maxShapeRadius),
													  Randf(minShapeRadius, maxShapeRadius));

							AddBox(extents, shapeTranslation, shapeRotation);
							break;
						}
						case 3:
						{
							AddRandomConvex(6 + Rand()%6, shapeTranslation, minShapeRadius, maxShapeRadius, UniformSampleSphere(), Randf()*k2Pi);							
							break;
						}
						case 4:
						{
							AddTriangleMesh(mesh, shapeTranslation, shapeRotation, Randf(0.5f, 1.0f)*maxShapeRadius);
							break;
						}
						case 5:
						{
							AddSDF(sdf, shapeTranslation, shapeRotation, maxShapeRadius*2.0f);
							break;
						}
					};
				}
			}
		}
	}
		
	virtual void CenterCamera()
	{
		g_camPos.y = 2.0f;
		g_camPos.z = 6.0f;
	}

};