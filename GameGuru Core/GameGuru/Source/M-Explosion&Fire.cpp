//----------------------------------------------------
//--- GAMEGURU - M-Explosion&Fire
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

// 
//  Explosions and fire
// 

void explosion_init ( void )
{
	#ifdef WICKEDENGINE
	// legacy explosion system only used for legacy particles, now uses particleresult system for explosions
	g.sparks = 12 + g.explosionsandfireimagesoffset;
	LoadImage ("effectbank\\explosion\\animatedspark.dds", g.sparks, 1);
	// up to 20 emitters for MANY explosions at once, and only need X parts per emitter
	g.maxemit = 20;
	g.totalpart = 100;
	#else
	// Stock explosion images
	g.rubbletext=15+g.explosionsandfireimagesoffset;
	g.cretetext=16+g.explosionsandfireimagesoffset;
	g.metaltext=17+g.explosionsandfireimagesoffset;
	g.largeexplosion=10+g.explosionsandfireimagesoffset;
	g.sparks=12+g.explosionsandfireimagesoffset;
	g.largeexplosion2=13+g.explosionsandfireimagesoffset;
	g.smokedecal2=14+g.explosionsandfireimagesoffset;
	g.rollingsmoke=20+g.explosionsandfireimagesoffset;
	g.grenadeexplosion=21+g.explosionsandfireimagesoffset;

	// Stock explosion objects
	g.rubbleobj=1+g.explosionsandfireobjectoffset;
	g.creteobj=2+g.explosionsandfireobjectoffset;
	g.metalobj=3+g.explosionsandfireobjectoffset;

	// Max Debris
	g.debrismax = 5;
	g.explosiondebrisobjectstart = 2700 + g.explosionsandfireobjectoffset;

	// Explosion art
	LoadImage ("effectbank\\explosion\\animatedspark.dds", g.sparks, 1);
	LoadImage ("effectbank\\explosion\\explosion2.dds", g.largeexplosion, 1);
	LoadImage ("effectbank\\explosion\\fireball.dds", g.largeexplosion2, 1);
	if (t.game.runasmultiplayer == 1) mp_refresh ();
	LoadImage ("effectbank\\explosion\\rollingsmoke.dds", g.rollingsmoke, 1);
	LoadImage ("effectbank\\explosion\\explosion3.dds", g.grenadeexplosion, 1);
	LoadImage ("effectbank\\explosion\\darksmoke.dds", g.smokedecal2, 1);
	// Maxemit=10 emitters, with totalpart of 260 for each emitter.
	g.totalpart = 260;
	g.maxemit = 5;
	#endif

	// place holder object pointers
	g.explosionparticleobjectstart = 20 + g.explosionsandfireobjectoffset;

	// Temp rubble
	#ifdef VRTECH
	 #ifdef WICKEDENGINE
	 // not used as far as I know
	 #else
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 LoadImage ( "effectbank\\explosion\\rubble.dds",g.rubbletext,1 );
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 LoadImage (  "effectbank\\explosion\\concretechunk.dds",g.cretetext,1 );
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 LoadImage (  "effectbank\\explosion\\metalchunk.dds",g.metaltext,1 );
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 MakeObjectBox(g.rubbleobj, 0, 0, 0);
	 TextureObject (  g.rubbleobj,g.rubbletext );
	 HideObject (  g.rubbleobj );
	 MakeObjectBox(g.creteobj, 0, 0, 0);
	 TextureObject (  g.creteobj,g.cretetext );
	 HideObject (  g.creteobj );
	 MakeObjectBox(g.metalobj, 0, 0, 0);
	 TextureObject (  g.metalobj,g.metaltext );
	 HideObject (  g.metalobj );
     #endif
	#else
	 LoadImage (  "effectbank\\explosion\\rubble.dds",g.rubbletext,1 );
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 LoadObject (  "effectbank\\explosion\\rubble.dbo",g.rubbleobj );
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 TextureObject (  g.rubbleobj,g.rubbletext );
	 ScaleObject (  g.rubbleobj,100,100,100 );
	 HideObject (  g.rubbleobj );
	 LoadImage (  "effectbank\\explosion\\concretechunk.dds",g.cretetext,1 );
	 LoadObject (  "effectbank\\explosion\\concretechunk.dbo",g.creteobj );
	 if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	 TextureObject (  g.creteobj,g.cretetext );
	 HideObject (  g.creteobj );
	 LoadImage (  "effectbank\\explosion\\metalchunk.dds",g.metaltext,1 );
	 LoadObject (  "effectbank\\explosion\\metalchunk.dbo",g.metalobj );
	 TextureObject (  g.metalobj,g.metaltext );
	 HideObject (  g.metalobj );
	#endif

	//  debris data
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	Dim (  t.debris,g.debrismax  );
	Dim2(  t.particle,g.maxemit, g.totalpart);
	make_particles();
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	#ifdef WICKEDENGINE
	// no debris in wicked
	#else
	make_debris();
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	#endif
}

void explosion_cleanup ( void )
{
	#ifdef WICKEDENGINE
	// no debris in wicked
	#else
	//  remove debris
	for ( t.draw = 1 ; t.draw<=  g.debrismax; t.draw++ )
	{
		if (  t.draw <= ArrayCount(t.debris) ) 
		{
			if (  t.debris[t.draw].used == 1 && ObjectExist(t.debris[t.draw].obj) == 1 ) 
			{
				t.debris[t.draw].used=0;
				if (  t.debris[t.draw].physicscreated == 1 ) 
				{
					ODEDestroyObject (  t.debris[t.draw].obj );
					t.debris[t.draw].physicscreated=0;
				}
				HideObject (  t.debris[t.draw].obj );
			}
		}
	}
	#endif

	//  remove particles
	for ( t.emitter = 1 ; t.emitter<=  g.maxemit; t.emitter++ )
	{
		if (  t.emitter <= ArrayCount(t.particle) ) 
		{
			for ( t.draw = 1 ; t.draw<=  g.totalpart; t.draw++ )
			{
				//  check if particle used
				if (  t.particle[t.emitter][t.draw].used != 0 ) 
				{
					//  kill particle
					t.particle[t.emitter][t.draw].used = 0;
					if (  t.particle[t.emitter][t.draw].physicscreated == 1 ) 
					{
						ODEDestroyObject (  t.particle[t.emitter][t.draw].obj );
						t.particle[t.emitter][t.draw].physicscreated=0;
					}
					HideObject (  t.particle[t.emitter][t.draw].obj );
				}
			}
		}
	}
}

void explosion_free ( void )
{
}

#ifdef WICKEDENGINE
// no debris in wicked
#else
void draw_debris ( void )
{
	int tx = 0;
	int ty = 0;
	int tz = 0;
	int draw = 0;
	//  Draw debris and update, run through debris
	for ( draw = 1 ; draw <= g.debrismax; draw++ )
	{
		if (  t.debris[draw].used == 1 && ObjectExist(t.debris[draw].obj) == 1 ) 
		{
			//  if active
			PositionObject (  t.debris[draw].obj,t.debris[draw].x,t.debris[draw].y,t.debris[draw].z );
			ShowObject (  t.debris[draw].obj );
			//  set to spinning
			tx=WrapValue(ObjectAngleX(t.debris[draw].obj)+5);
			ty=WrapValue(ObjectAngleY(t.debris[draw].obj)+5);
			tz=WrapValue(ObjectAngleZ(t.debris[draw].obj)+5);
			RotateObject (  t.debris[draw].obj,tx,ty,tz );
			//  kill if life expired
			if (  Timer()-t.debris[draw].lifetime>t.debris[draw].life ) 
			{
				t.debris[draw].used=0;
				if (  t.debris[draw].physicscreated == 1 ) 
				{
					ODEDestroyObject (  t.debris[draw].obj );
					t.debris[draw].physicscreated=0;
				}
				HideObject (  t.debris[draw].obj );
			}
		}
	}
}
#endif

void draw_particles ( void )
{
	int tonceforemitter = 0;
	int hideframe = 0;
	int endframe = 0;
	int emitter = 0;
	int doit = 0;
	int size = 0;
	int tobj = 0;
	int zz = 0;
	int draw = 0;
	for ( emitter = 1 ; emitter<=  g.maxemit; emitter++ )
	{
		tonceforemitter=0;
		for ( draw = 1 ; draw<=  g.totalpart; draw++ )
		{
			//  check if particle used
			if (  t.particle[emitter][draw].used != 0 ) 
			{
				// if determined for use, create it now
				#ifdef WICKEDENGINE
				int iObjID = t.particle[emitter][draw].obj;
				if (iObjID > 0)
				{
					sObject* pObject = GetObjectData(iObjID);
					if ( pObject )
					{
						if (pObject->wickedrootentityindex == 0)
						{
							WickedCall_AddObject(pObject);
							WickedCall_TextureObject(pObject,NULL);
							WickedCall_SetObjectTransparent(pObject);
						}
					}
				}
				#endif

				//  check for delayed particles
				doit=1;
				if (  t.particle[emitter][draw].activein != 0 ) 
				{
					zz=Timer()-t.particle[emitter][draw].activetime;
					if (  zz<t.particle[emitter][draw].activein  )  doit = 0;
				}

				//  if active but dead, ensure particle is killed
				if (  doit == 1 ) 
				{

					//  size is the number of frames in an animation, current 4x4 and 8x8 supported.
					endframe=0;
					if (  t.particle[emitter][draw].size == 4 ) 
					{
						endframe=16;
					}
					else
					{
						endframe=64;
					}
					size=t.particle[emitter][draw].size;

					//  if explosion show quick flash, in full version should be replaced with FPSC spotflash.
					if (  t.particle[emitter][draw].etype == 2 || t.particle[emitter][draw].etype == 7 || t.particle[emitter][draw].etype == 6 ) 
					{
						//  Show explosion light
					}

					//  control alpha and apply new value if it's changed
					t.particle[emitter][draw].oldalpha=t.particle[emitter][draw].alpha;
					if (  t.particle[emitter][draw].fadein != 0 ) 
					{
						if (  t.particle[emitter][draw].fadedir == 0 ) 
						{
							t.particle[emitter][draw].alpha=t.particle[emitter][draw].alpha+t.particle[emitter][draw].fadein;
							if (  t.particle[emitter][draw].alpha>t.particle[emitter][draw].fademax ) 
							{
								t.particle[emitter][draw].alpha=t.particle[emitter][draw].fademax;
								if (  t.particle[emitter][draw].fadeout>0  )  t.particle[emitter][draw].fadedir = 1;
							}
							if (  t.particle[emitter][draw].alpha<0 && t.particle[emitter][draw].fadein != 0  )  t.particle[emitter][draw].alpha = 0;
						}
						if (  t.particle[emitter][draw].fadedir == 1 ) 
						{
							t.particle[emitter][draw].alpha=t.particle[emitter][draw].alpha-t.particle[emitter][draw].fadeout;
							if (  t.particle[emitter][draw].alpha<0 ) 
							{
								t.particle[emitter][draw].alpha=0;
								t.particle[emitter][draw].fadedir=2;
							}
						}
					}
					if (  t.particle[emitter][draw].oldalpha != t.particle[emitter][draw].alpha ) 
					{
						SetAlphaMappingOn (  t.particle[emitter][draw].obj,t.particle[emitter][draw].alpha );
					}
					//  if particle is a fire, generate sparks every 2.5 seconds
					if (  t.particle[emitter][draw].etype == 5 || t.particle[emitter][draw].etype == 12 || t.particle[emitter][draw].etype == 13 ) 
					{
						if (  Timer()-t.particle[emitter][draw].time>2500 ) 
						{
							Create_Emitter(t.particle[emitter][draw].x,t.particle[emitter][draw].y,t.particle[emitter][draw].z,14,Rnd(2)+1,g.sparkdecal,0,0,0,0,0,0);
							t.particle[emitter][draw].time=Timer();
							if (  t.particle[emitter][draw].etype == 5 && t.particle[emitter][draw].nextframe>35 ) 
							{
								make_large_fire(t.particle[emitter][draw].x,t.particle[emitter][draw].y-20,t.particle[emitter][draw].z);
							}
						}
					}
					//  move particles using at pre designed speed allowing for a little chaos. Fires have no movement
					if (  t.particle[emitter][draw].etype != 5 && t.particle[emitter][draw].etype != 11 && t.particle[emitter][draw].etype != 12 && t.particle[emitter][draw].etype != 13 ) 
					{
						t.particle[emitter][draw].x=t.particle[emitter][draw].x+t.particle[emitter][draw].vx+Rnd(t.particle[emitter][draw].vxvar_f);
						t.particle[emitter][draw].y=t.particle[emitter][draw].y+t.particle[emitter][draw].vy+Rnd(t.particle[emitter][draw].vyvar_f);
						t.particle[emitter][draw].z=t.particle[emitter][draw].z+t.particle[emitter][draw].vz+Rnd(t.particle[emitter][draw].vzvar_f);
					}
					//  update
					PositionObject (  t.particle[emitter][draw].obj,t.particle[emitter][draw].x,t.particle[emitter][draw].y,t.particle[emitter][draw].z );
					//  rem point at camera
					tobj=t.particle[emitter][draw].obj;
					PointObject (  tobj,CameraPositionX(0),CameraPositionY(0),CameraPositionZ(0) );
					//C++ISSUE - zbias doesnt seem to work for this, so set it to -1 for now
					//EnableObjectZBias (  tobj,0.0f,-1.0f );
					// this causes explosion to render in front of things when it should be behind/amongst (like grass)
					// MoveObject ( tobj , 200 );
					hideframe=0;
					if (  t.particle[emitter][draw].etype == 2 && t.particle[emitter][draw].nextframe == 1  )  hideframe = 1;
					if (  hideframe == 0  )  ShowObject (  t.particle[emitter][draw].obj );
					//  update animation frame if time.
					if (  Timer()-t.particle[emitter][draw].lastanitime>t.particle[emitter][draw].anispeed ) 
					{
						t.particle[emitter][draw].lastanitime=Timer();
						Set_Object_Frame_Update(t.particle[emitter][draw].obj,t.particle[emitter][draw].nextframe,size,size);
						++t.particle[emitter][draw].nextframe;
						//  reset if animation at last frame, kill if not looped
						if (  t.particle[emitter][draw].nextframe >= endframe ) 
						{
							if (  t.particle[emitter][draw].playforloops == 0 || (t.particle[emitter][draw].alpha <= 0 && t.particle[emitter][draw].fadein<0) ) 
							{
								t.particle[emitter][draw].used=0;
								t.particle[emitter][draw].nextframe=0;
							}
							else
							{
								--t.particle[emitter][draw].playforloops;
								t.particle[emitter][draw].nextframe=1;
								if (  t.particle[emitter][draw].playforloops<3 && (t.particle[emitter][draw].etype == 5 || t.particle[emitter][draw].etype == 12 || t.particle[emitter][draw].etype == 13 || t.particle[emitter][draw].etype == 14) || (t.particle[emitter][draw].playforloops == 0 && t.particle[emitter][draw].etype == 1) ) 
								{
									//  control alpha fading if on way out. Small fire fades faster
									if (  t.particle[emitter][draw].etype != 14 ) 
									{
										t.particle[emitter][draw].fadein=- 0.3f;
									}
									else
									{
										t.particle[emitter][draw].fadein=-0.5f;
									}
								}
							}
						}
					}

					//  if particle has a life counter, kill when exceeds it
					if (  t.particle[emitter][draw].life != 0 ) 
					{
						if (  Timer()>t.particle[emitter][draw].life ) 
						{
							t.particle[emitter][draw].used=0;
						}
					}
					if (  t.particle[emitter][draw].alpha <= 0 && t.particle[emitter][draw].fadein != 0 ) 
					{
						t.particle[emitter][draw].used=0;
					}

					//  hide/destroy if no longer used
					if (  t.particle[emitter][draw].used == 0 ) 
					{
						//  kill particle
						if (  t.particle[emitter][draw].physicscreated == 1 ) 
						{
							ODEDestroyObject (  t.particle[emitter][draw].obj );
							t.particle[emitter][draw].physicscreated=0;
						}
						HideObject (  t.particle[emitter][draw].obj );
					}

				}
				//  particle used endif
			}
		}
	}
}

//  create emitters using presets.
int Create_Emitter ( int x, int y, int z, int etype, int part, int textureid, int delay, int scale, int damage, int sound, int volume, int loopsound )
{
	float variation_f = 0;
	int forceangle = 0;
	float funangle_f = 0;
	float tangle_f = 0;
	float tforce_f = 0;
	int emitter = 0;
	float fundx_f = 0;
	float fundy_f = 0;
	int num = 0;
	int use = 0;

	// 280618 - ensure scale defaults to normal
	if ( scale == 0 ) scale = 100;

	// find free emitter, if none free exit
	emitter = find_free_emitter();
	if ( emitter == 0 ) return 0;

	// create particle emitter based on type
	switch ( etype ) 
	{
		// 1 = make small smoke
		case 1:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,1,10);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=0;
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x+(Sin(variation_f)*10);
							t.particle[emitter][use].y=y+(num*(Rnd(32)+2));
							t.particle[emitter][use].z=z+(Cos(variation_f)*10);
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=1;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=60;
							TextureObject (  t.particle[emitter][use].obj,0,textureid );
							t.particle[emitter][use].nextframe=1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,0 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							t.particle[emitter][use].vx=-0.02f;
							t.particle[emitter][use].vy=1+(Rnd(0.2f)+(num*0.07f));
							t.particle[emitter][use].vz=0.001f;
							t.particle[emitter][use].vxvar_f=0.04f;
							t.particle[emitter][use].vyvar_f=0.05f+(num*0.02f);
							t.particle[emitter][use].vzvar_f=0.002f;
							t.particle[emitter][use].alpha=0;
							t.particle[emitter][use].fadein=0.6f;
							t.particle[emitter][use].fademax=255;
							t.particle[emitter][use].fadedir=0;
							t.particle[emitter][use].fadeout=0;
							t.particle[emitter][use].activeframe=Rnd(62)+1;
						}
					}
				}
			}
		break;

		// 4 = sparks from explosion
		case 4:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,41,110);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=0;
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x+(Sin(variation_f)*8);
							t.particle[emitter][use].y=y;
							t.particle[emitter][use].z=z+(Cos(variation_f)*8);
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].size=4;
							t.particle[emitter][use].etype=7;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=4;
							t.particle[emitter][use].life=Timer()+1500;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=35;
							TextureObject (  t.particle[emitter][use].obj,0,textureid );
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].nextframe=1;
							forceangle=WrapValue(-90+Rnd(180));
							tforce_f=2.5f;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,30 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6);
							PositionObject ( t.particle[emitter][use].obj,t.particle[emitter][use].x,t.particle[emitter][use].y,t.particle[emitter][use].z );
							ScaleObject ( t.particle[emitter][use].obj, scale, scale, scale );
							if (  t.particle[emitter][use].physicscreated == 1  )  ODEDestroyObject (  t.particle[emitter][use].obj );
							t.particle[emitter][use].physicscreated=1;
							ODECreateDynamicBox (  t.particle[emitter][use].obj,-1,11 );
							ODESetLinearVelocity (  t.particle[emitter][use].obj,(Rnd(25))*tforce_f,(30+Rnd(205))*tforce_f,(-Rnd(25)+Rnd(25))*tforce_f );
							ODESetAngularVelocity (  t.particle[emitter][use].obj,0,forceangle,0 );
							ODEAddBodyForce (  t.particle[emitter][use].obj, 0, 0.05f, 0,0,-50,0 );
						}
					}
				}
			}
		break;

		// 5=  fire large
		case 5:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,111,116);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=2+Rnd(2);
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x;
							t.particle[emitter][use].y=y;
							t.particle[emitter][use].z=z;
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=5;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=20;
							TextureObject (  t.particle[emitter][use].obj,0,textureid );
							t.particle[emitter][use].nextframe=Rnd(8)+1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,0 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							t.particle[emitter][use].vy=0;
							t.particle[emitter][use].vz=0;
							t.particle[emitter][use].vz=0;
							t.particle[emitter][use].vxvar_f=0;
							t.particle[emitter][use].vyvar_f=0;
							t.particle[emitter][use].vzvar_f=0;
							t.particle[emitter][use].fadein=0.4f;
							t.particle[emitter][use].fademax=255;
							t.particle[emitter][use].fadedir=0;
							t.particle[emitter][use].fadeout=0;
							t.particle[emitter][use].alpha=0;
							t.particle[emitter][use].time=Timer()+Rnd(1000);
						}
					}
				}
			}
		break;

		// 6 = large explosion
		case 6:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,131,160);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=0;
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x;
							t.particle[emitter][use].y=y+(ObjectSize(t.particle[emitter][use].obj)/4);
							t.particle[emitter][use].z=z;
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=6;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=25;
							t.particle[emitter][use].fadein=10.0f;
							t.particle[emitter][use].fademax=100;
							t.particle[emitter][use].fadedir=0;
							t.particle[emitter][use].fadeout=1.0f;
							TextureObject (  t.particle[emitter][use].obj,0,textureid );
							t.particle[emitter][use].nextframe=1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,100 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							t.particle[emitter][use].vx=-0.001f;
							t.particle[emitter][use].vy=0.001f+(num*0.002f);
							t.particle[emitter][use].vz=-0.002f;
							t.particle[emitter][use].vxvar_f=0.002f;
							t.particle[emitter][use].vyvar_f=0.003f;
							t.particle[emitter][use].vzvar_f=-0.01f;
						}
					}
				}
			}
		break;

		// 12 = flame meduim
		case 12:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,117,122);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=2+Rnd(2);
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x+(Sin(variation_f)*15);
							t.particle[emitter][use].y=y;
							t.particle[emitter][use].z=z+(Cos(variation_f)*15);
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=12;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=20;
							TextureObject (  t.particle[emitter][use].obj,0,textureid );
							t.particle[emitter][use].nextframe=Rnd(8)+1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,0 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							t.particle[emitter][use].vy=0;
							t.particle[emitter][use].vx=0;
							t.particle[emitter][use].vz=0;
							t.particle[emitter][use].vxvar_f=0;
							t.particle[emitter][use].vyvar_f=0;
							t.particle[emitter][use].vzvar_f=0;
							t.particle[emitter][use].fadein=0.3f;
							t.particle[emitter][use].fademax=255;
							t.particle[emitter][use].fadedir=0;
							t.particle[emitter][use].fadeout=0;
							t.particle[emitter][use].alpha=0;
							t.particle[emitter][use].time=Timer()+Rnd(1000);
						}
					}
				}
			}
		break;

		// 13 = large flame
		case 13:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,123,130);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=2+Rnd(2);
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x+(Sin(variation_f)*7);
							t.particle[emitter][use].y=y;
							t.particle[emitter][use].z=z+(Cos(variation_f)*7);
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=13;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=20;
							TextureObject (  t.particle[emitter][use].obj,0,textureid );
							t.particle[emitter][use].nextframe=Rnd(8)+1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,0 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							t.particle[emitter][use].vy=0;
							t.particle[emitter][use].vx=0;
							t.particle[emitter][use].vz=0;
							t.particle[emitter][use].vxvar_f=0;
							t.particle[emitter][use].vyvar_f=0;
							t.particle[emitter][use].vzvar_f=0;
							t.particle[emitter][use].fadein=0.3f;
							t.particle[emitter][use].fademax=255;
							t.particle[emitter][use].fadedir=0;
							t.particle[emitter][use].fadeout=0;
							t.particle[emitter][use].alpha=0;
							t.particle[emitter][use].time=Timer()+Rnd(1000);
						}
					}
				}
			}
		break;

		// 15 = large smoke
		case 15:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,11,40);
				if (  use != 0 ) 
				{
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=0;
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x+(Sin(variation_f)*1);
							t.particle[emitter][use].y=y+(num*(Rnd(32)+2));
							t.particle[emitter][use].z=z;
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=1;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=35;
							TextureObject (  t.particle[emitter][use].obj, 0, textureid );
							ScaleObject ( t.particle[emitter][use].obj, scale, scale, scale );
							t.particle[emitter][use].nextframe=1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj, 0 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							//PE: We need to set this type to 15.
							SetObjectDebugInfo(t.particle[emitter][use].obj, 15);
							t.particle[emitter][use].vx=-0.02f;
							t.particle[emitter][use].vy=0.8f+Rnd(0.2f)+(num*0.03f);
							t.particle[emitter][use].vz=0.001f;
							t.particle[emitter][use].vxvar_f=0.04f;
							t.particle[emitter][use].vyvar_f=0.5f+(num*0.02f);
							t.particle[emitter][use].vzvar_f=0.002f;
							t.particle[emitter][use].alpha=0;
							t.particle[emitter][use].fadein=1.5f;
							t.particle[emitter][use].fademax=255;
							t.particle[emitter][use].fadedir=0;
							t.particle[emitter][use].fadeout=0;
						}
					}
				}
			}
		break;

		// 16 = large explosion
		case 16:
			for ( num = 1 ; num<=  part; num++ )
			{
				use=find_free_particle(emitter,1,9);
				if (  use != 0 ) 
				{
					reset_current_particle(emitter,use);
					if (  t.particle[emitter][use].obj>0 ) 
					{
						if (  ObjectExist(t.particle[emitter][use].obj) == 1 ) 
						{
							variation_f=Rnd(360);
							t.particle[emitter][use].playforloops=0;
							t.particle[emitter][use].actionperformed=0;
							t.particle[emitter][use].damage=damage;
							t.particle[emitter][use].x=x;
							t.particle[emitter][use].y=y;
							t.particle[emitter][use].z=z;
							t.particle[emitter][use].used=1;
							t.particle[emitter][use].etype=16;
							t.particle[emitter][use].size=8;
							t.particle[emitter][use].activein=delay;
							t.particle[emitter][use].activetime=Timer();
							t.particle[emitter][use].rotate=0;
							t.particle[emitter][use].life=0;
							t.particle[emitter][use].lasttime=Timer();
							t.particle[emitter][use].lastanitime=Timer();
							t.particle[emitter][use].anispeed=25;
							TextureObject ( t.particle[emitter][use].obj,0,textureid );
							ScaleObject ( t.particle[emitter][use].obj, scale, scale, scale );
							t.particle[emitter][use].nextframe=1;
							SetAlphaMappingOn (  t.particle[emitter][use].obj,100 );
							SetObjectTransparency (  t.particle[emitter][use].obj,6 );
							t.particle[emitter][use].vx=-0.001f;
							t.particle[emitter][use].vy=0.001f+(num*0.002f);
							t.particle[emitter][use].vz=-0.002f;
							t.particle[emitter][use].vxvar_f=0.002f;
							t.particle[emitter][use].vyvar_f=0.003f;
							t.particle[emitter][use].vzvar_f=-0.01f;
						}
					}
				}
			}
		break;
	}
	return emitter;
}

void Set_Object_Frame(int tempobj, int currentframe, int height_f, int width_f)
{
	#ifdef WICKEDENGINE
	// adjust UV manually until GPU particles come along
	SetObjectUVManually(tempobj, currentframe, width_f, height_f);
	#else
	// not sure this is right (or used anymore)
	float U_f = 0;
	float V_f = 0;
	float xmove_f = 1.0f / height_f;
	float ymove_f = 1.0f / width_f;
	LockVertexDataForLimb(tempobj, 0);
	SetVertexDataUV(0, U_f + xmove_f, V_f);
	SetVertexDataUV(1, U_f, V_f);
	SetVertexDataUV(2, U_f + xmove_f, V_f + ymove_f);
	SetVertexDataUV(3, U_f, V_f);
	SetVertexDataUV(4, U_f, V_f + ymove_f);
	SetVertexDataUV(5, U_f + xmove_f, V_f + ymove_f);
	UnlockVertexData();
	// reset effect UV scaling
	ScaleObjectTexture (  tempobj,0,0 );
	#endif
}

void Set_Object_Frame_Update ( int tempobj, int currentframe, int height_f, int width_f )
{
	#ifdef WICKEDENGINE
	// going to replace with new GPU particle system, so for now just write the correct UV data and
	// update the wicked mesh to reflect the animation of this decal/plane
	Set_Object_Frame(tempobj, currentframe, height_f, width_f);
	#else
	float xmove_f = 0;
	float ymove_f = 0;
	int across = 0;
	float U_f = 0;
	float V_f = 0;
	xmove_f=1.0/height_f;
	ymove_f=1.0/width_f;
	if ( currentframe != 0 )
	{
		across=int(currentframe/height_f);
		V_f=ymove_f*across;
		U_f=currentframe*xmove_f;
	}
	else
	{
		U_f=0;
		V_f=0;
	}
	// replace vertex data write with special feed into shader (values write to shader constant UVScaling)
	ScaleObjectTexture (  tempobj,U_f,V_f );
	#endif
}

#ifdef WICKEDENGINE
// no debris in wicked
#else
void make_debris ( void )
{
	// make and intilise debris objects
	int nextobject = 0;
	int rand = 0;
	int make = 0;
	for ( make = 1 ; make <= g.debrismax; make++ )
	{
		nextobject=g.explosiondebrisobjectstart+make;
		rand=g.rubbleobj+Rnd(2);
		CloneObject ( nextobject,rand );
		PositionObject ( nextobject,0,0,0 );
		ScaleObject ( nextobject,Rnd(20)+15,Rnd(20)+15,Rnd(20)+15 );
		t.debris[make].obj=nextobject;
		t.debris[make].used=0;
		HideObject ( nextobject );
	}
}
#endif

void make_particles ( void )
{
	int nextobject = 0;
	nextobject = g.explosionparticleobjectstart;
	#ifdef WICKEDENGINE
	// quit is ALREADY created these - it seems!
	if (ObjectExist(nextobject) == 1)
	{
		return;
	}
	#endif

	#ifdef WICKEDENGINE
	#ifdef _DEBUG
	// limit emitters quantity in debug mode (too slow to create all scene elements in debug?!)
	// eventually replace all this with a GPU based particle system (no more eating wicked resources)
	g.maxemit = 1;
	#endif
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
	WickedCall_PresetObjectCreateOnDemand(true);
	#endif

	// make and intilise particle objects, different types for different tasks.
	int emitter = 0;
	int tscale = 0;
	int scale = 0;
	int make = 0;
	for ( emitter = 1 ; emitter <= g.maxemit; emitter++ )
	{
		for ( make = 1 ; make <= g.totalpart; make++ )
		{
			++nextobject;
			scale=180+Rnd(80);

			if (ObjectExist(nextobject) == 0) //PE: Already exists ?
			{
				//  Explosion particles
				if (make <= 10)
				{
					MakeObjectPlane(nextobject, 200, 200);
				}
				//  Major smoke
				if (make > 10 && make <= 40)
				{
					MakeObjectPlane(nextobject, scale, scale);
				}
				if (make > 40 && make <= 80)
				{
					MakeObjectPlane(nextobject, 2 + Rnd(2), 2 + Rnd(2));
				}
				if (make > 80 && make <= 110)
				{
					tscale = 1 + Rnd(1);
					MakeObjectPlane(nextobject, tscale, tscale);
				}
				if (make > 110 && make <= 116)
				{
					MakeObjectPlane(nextobject, 90, 70);
				}
				if (make > 116 && make <= 122)
				{
					MakeObjectPlane(nextobject, 60, 60);
				}
				if (make > 122 && make <= 130)
				{
					MakeObjectPlane(nextobject, 30, 30);
				}
				//  big bang
				if (make > 130 && make <= 160)
				{
					MakeObjectPlane(nextobject, 400, 400);
				}
				if (make > 160 && make <= 250)
				{
					MakeObjectPlane(nextobject, 10, 10);
				}
				if (make > 250 && make <= 261)
				{
					MakeObjectPlane(nextobject, 400, 200);
				}
			}
			TextureObject ( nextobject,g.smokedecal );
			SetObjectTransparency ( nextobject, 6 );

			// apply decal shader
			SetObjectEffect ( nextobject, g.decaleffectoffset );
			SetObjectCull ( nextobject, 0 );
			DisableObjectZWrite ( nextobject );
			SetObjectMask ( nextobject, 1 );
			Set_Object_Frame(nextobject,0,8,8);
			HideObject (  nextobject );
			t.particle[emitter][make].obj=nextobject;
		}
	}

	#ifdef WICKEDENGINE
	WickedCall_PresetObjectCreateOnDemand(false);
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
	#endif
}

#ifdef WICKEDENGINE
// no debris in wicked
#else
int find_free_debris ( void )
{
	int gotone = 0;
	int find = 0;
	gotone=0;
	for ( find = 1 ; find<=  30; find++ )
	{
		if (  t.debris[find].used == 0 ) 
		{
			t.debris[find].used=1;
			gotone=find;
			find=31;
		}
	}
	return gotone;
}
#endif

int find_free_particle ( int emitter, int start, int endpart )
{
	int gotone = 0;
	int find = 0;
	gotone=0;
	for ( find = start ; find<=  endpart; find++ )
	{
		if (  t.particle[emitter][find].used == 0 ) 
		{
			gotone=find;
			find=endpart+1;
		}
	}
	return gotone;
}

int find_free_emitter ( void )
{
	int gotone = 0;
	int find = 0;
	gotone=0;	
	for ( find = 1 ; find<=  g.maxemit; find++ )
	{
		if (  t.particle[find][1].used == 0 ) 
		{
			gotone=find;
			find=g.maxemit+1;
		}
	}
	return gotone;
}

void make_large_fire ( int x, int y, int z )
{
	int firesprite = 0;
	int firetotal = 0;
	int tx = 0;
	int tz = 0;
	int fire = 0;
	firesprite=0;
	firetotal=1;
	for ( fire = 1 ; fire<=  firetotal; fire++ )
	{
		tx=x;
		tz=z;
		Create_Emitter(tx+8,y,tz,13,1,g.afterburndecal+firesprite,0,0,0,5,100,1);
		Create_Emitter(tx,y+12,tz,12,1,g.afterburndecal+firesprite,500,0,20,0,0,0);
		Create_Emitter(tx,y+20,tz,5,1,g.afterburndecal+firesprite,1000,0,20,5,100,1);
		Create_Emitter(x+8,y+20,z,12,2+Rnd(1),g.afterburndecal+firesprite,1000,0,0,5,100,1);
	}
	//  create smoke randomly
	if (  Rnd(100)>50 ) 
	{
		Create_Emitter(x,y+35,z,1,1,g.smokedecal2,700,0,0,0,0,0);
	}
}

void explosion_rocket ( int x, int y, int z )
{
	int f = 0;
	//  explosion
	Create_Emitter(x-Rnd(2),y+50,z,6,2,g.largeexplosion,0,0,20,2,100,0);
	//  sparks
	for ( f = 0 ; f <= 4; f++ )
	{
		Create_Emitter(x,y+30,z,4,16,g.sparks,70+(f*500),0,0,0,0,0);
	}
}

void explosion_fireball ( int x, int y, int z )
{
	int temitterindex = 0;
	int tmodeindex = 0;
	int f = 0;
	// explosion
	temitterindex=Create_Emitter(x-Rnd(2),y,z,16,1,g.grenadeexplosion,80,0,20,2,100,0);
	// dynamic light blast
	t.playerlight.mode=0 ; t.tx_f=x ; t.ty_f=y ; t.tz_f=z ; tmodeindex=temitterindex ; lighting_spotflashexplosion ( );
	// explosion second smoke
	Create_Emitter(x,y+10,z,15,1,g.smokedecal2,60,0,20,0,100,0);
	// sparks
	for ( f = 0 ; f <= 7; f++ )
	{
		Create_Emitter(x,y,z,4,5,g.sparks,70+(f*500),0,0,0,0,0);
	}
}

void explosion_custom ( int imageID, int iLightFlag, int iSmokeImageID, int iSharksCount, float fSize, float fSmokeSize, float fSparksSize, int x, int y, int z )
{
	int temitterindex = 0;
	int tmodeindex = 0;
	int f = 0;
	if ( imageID > 0 )
	{
		// explosion
		temitterindex = Create_Emitter(x-Rnd(2),y,z,16,1,imageID,1,100.0f*fSize,20,2,100,0);
	}
	if ( iLightFlag != 0 )
	{
		// dynamic light blast
		t.playerlight.mode=0 ; t.tx_f=x ; t.ty_f=y ; t.tz_f=z; 
		tmodeindex=temitterindex; 
		lighting_spotflashexplosion ( );
	}
	if ( iSmokeImageID != 0 )
	{
		// explosion second smoke
		if ( iSmokeImageID == -1 ) iSmokeImageID = g.smokedecal2;
		Create_Emitter(x,y+10,z,15,1,iSmokeImageID,60,100.0f*fSmokeSize,20,0,100,0);
	}
	if ( iSharksCount > 0 )
	{
		// sparks
		for ( f = 0 ; f <= iSharksCount; f++ )
		{
			Create_Emitter(x,y,z,4,5,g.sparks,70+(f*500),100.0f*fSparksSize,0,0,0,0);
		}
	}
}

void reset_current_particle ( int emitter, int use )
{
	int damage = 0;
	t.particle[emitter][use].actionperformed=0;
	t.particle[emitter][use].damage=damage;
	t.particle[emitter][use].x=-100000;
	t.particle[emitter][use].y=-100000;
	t.particle[emitter][use].z=-100000;
	t.particle[emitter][use].used=0;
	t.particle[emitter][use].etype=0;
	t.particle[emitter][use].size=0;
	t.particle[emitter][use].activein=0;
	t.particle[emitter][use].activetime=Timer();
	t.particle[emitter][use].rotate=0;
	t.particle[emitter][use].life=0;
	t.particle[emitter][use].lasttime=Timer();
	t.particle[emitter][use].lastanitime=Timer();
	t.particle[emitter][use].anispeed=0;
	t.particle[emitter][use].nextframe=0;
	t.particle[emitter][use].vx=0;
	t.particle[emitter][use].vy=0;
	t.particle[emitter][use].vz=0;
	t.particle[emitter][use].vxvar_f=0;
	t.particle[emitter][use].vyvar_f=0;
	t.particle[emitter][use].vzvar_f=0;
	t.particle[emitter][use].alpha=0;
	t.particle[emitter][use].fadein=0;
	t.particle[emitter][use].fademax=0;
	t.particle[emitter][use].fadedir=0;
	t.particle[emitter][use].fadeout=0;
	t.particle[emitter][use].activeframe=0;
	t.particle[emitter][use].oldalpha=0;
}

