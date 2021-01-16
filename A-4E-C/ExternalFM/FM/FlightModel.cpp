#include <vector>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include "FlightModel.h"
#include "Globals.h"
#include "Data.h"
#include "Maths.h"

#undef min

//Set everything to zero.
//Vectors already constructor to zero vector.
Skyhawk::FlightModel::FlightModel
(
	AircraftState& state,
	Input& controls,
	Airframe& airframe,
	Engine2& engine,
	Interface& inter
):
	m_state(state),
	m_controls(controls),
	m_airframe(airframe),
	m_engine(engine),
	m_interface( inter ),
	m_scalarVSquared(0.0),
	m_scalarV(0.0),
	m_q(0.0),
	m_p(0.0),
	m_k(0.0),
	m_aoaDot(0.0),
	m_aoaPrevious(0.0),
	m_LslatVel(0.0),
	m_RslatVel(0.0),
	m_cockpitShake(0.0),

	//m_elementLW(m_state, CLalpha, CDalpha, Vec3(0, 0, -3.0), m_wingSurfaceNormalL, m_totalWingArea/2),
	//m_elementRW(m_state, CLalpha, CDalpha, Vec3(0, 0, 3.0), m_wingSurfaceNormalR, m_totalWingArea/2),
	m_elementLSlat(m_state, dCLslat, CDslat, Vec3(0, 0, -3.0), m_wingSurfaceNormalL, m_totalWingArea / 2),
	m_elementRSlat(m_state, dCLslat, CDslat, Vec3(0, 0, 3.0), m_wingSurfaceNormalR, m_totalWingArea / 2),
	m_elementLFlap(m_state, dCLflap, CDflap, Vec3(0, 0, -3.0), m_wingSurfaceNormalL, m_totalWingArea / 2),
	m_elementRFlap(m_state, dCLflap, CDflap, Vec3(0, 0, 3.0), m_wingSurfaceNormalR, m_totalWingArea / 2),
	m_elementLSpoiler(m_state, dCLspoiler, dCDspoiler, Vec3(0, 0, -3.0), m_wingSurfaceNormalL, m_totalWingArea / 2),
	m_elementRSpoiler(m_state, dCLspoiler, dCDspoiler, Vec3(0, 0, 3.0), m_wingSurfaceNormalR, m_totalWingArea / 2),
	m_elementHorizontalStab(m_state, m_airframe, CLhstab, CDhstab, comp_e, Vec3(-5.1, 1.65, 0.0), m_hStabSurfaceNormal, 4.0),
	m_elementVerticalStab(m_state, m_airframe, CLvstab, CDvstab, Vec3(-4.8, 2.2, 0.0), m_vStabSurfaceNormal, 5.3),
	//m_elementLAil(m_state, dCLflap, CDflap, Vec3(0, 0, -3.2), m_wingSurfaceNormalL, m_totalWingArea/5),
	//m_elementRAil(m_state, dCLflap, CDflap, Vec3(0, 0, 3.2), m_wingSurfaceNormalR,  m_totalWingArea/5),

	//Setup tables
	//CLalpha(d_CLalpha, -0.26981317, 1.57079633), OLD - DONT DELETE
	//CLalpha(d_CLalpha, -1.5708, 1.5708),
	CLalpha(d_CLalpha, -PI, PI),
	dCLflap(d_CLflap, -0.26981317, 1.57079633),
	dCLslat(d_CLslat, -0.26981317, 1.57079633),
	dCLspoiler({-0.35}, 0, 1),
	CLde(d_CLde, 0.157982, 1.000377),
	//CDalpha(d_CDalpha, -1.57079633, 1.57079633),
	CDalpha(d_CDalpha, -PI, PI),
	CDi({0.025}, 0, 1),
	CDmach(d_CDmach,0.0, 1.8),
	CDflap(d_CDflap, -1.57079633, 1.57079633),
	CDslat(d_CDslat, -1.57079633, 1.57079633),
	dCDspoiler({ 0.05 }, 0, 1),
	dCDspeedBrake({0.08}, 0.0, 1.0),
	CDbeta(d_CDbeta,-1.57, 1.57),
	CDde({0.005}, c_elevatorDown, c_elevatorUp),

	//CLhstab(d_CL_hstab_alpha, -1.57079633, 1.57079633),
	CLhstab(d_CL_hstab_alpha, -PI, PI),
	CDhstab(d_CD_hstab_alpha, -PI, PI),
	//CLvstab(d_CL_vstab_aoa, -1.57079633, 1.57079633),
	CLvstab(d_CL_vstab_aoa, -PI, PI),
	CDvstab(d_CD_vstab_alpha, -PI, PI),

	comp_e(d_comp_e, 0.0, 1.0),

	CYb({-1}, 0.0, 1.0),

	Clb({ -0.01 }, 0.0, 1.0),
	Clp({ -0.0 }, 0.0, 1.0), //-0.3
	Clr({ 0.0 }, 0.0, 1.0), //0.15
	//Cla({ 0.220, 0.037 }, 0.0, 2.0), //110
	Cla({ 0.220 }, 0.0, 1.0),
	Cldr({ 0.0 }, 0.0, 1.0),
	Cla_a({1.0, 0.3}, 0.436332313, 0.698131701),

	CL_ail(d_CL_ail, 0.0, 0.872664626),
	CD_ail(d_CD_ail, 0.0, 0.872664626),

	Cmalpha(d_Cmalpha, 0.1, 1.0019),
	Cmde(d_Cmde, 0, 1.00216078), //x = mach
	Cmq(d_Cmq, 0.13176098, 1.0006616),
	Cmadot(d_Cmadot, 0.0, 1.0),
	CmM(d_CmM, 0.06761245, 1.0),
	Cmde_a({1.2, 0.4}, 0.34906585, 1.04719755),
	
	//Cnb({0.36}, 0.0, 1.0), //0.12
	Cnb(d_Cnb, -0.785398163, 0.785398163),
	Cnr({-0.08}, 0.0, 1.0), //-0.15
	Cndr({-0.1}, 0.0, 1.0),
	Cnda({0.2}, 0.0, 1.0),

	//slats
	slatCL({-0.15, 0.15}, 0.261799, 0.436332313),

	rnd_aoa(d_rnd_aoa, -0.34906585, 0.41887902)
{
	//m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, -0.1067866894), m_wingSurfaceNormalL, 1.9737953980 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, -0.5399751247), m_wingSurfaceNormalL, 1.8047195280 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, -0.9731336040), m_wingSurfaceNormalL, 1.6356436570 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, -1.4062517670), m_wingSurfaceNormalL, 1.4665677860 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, -1.8393138510), m_wingSurfaceNormalL, 1.2974919160 + 0.21931));
	//wing+ail								  
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, -2.2722946510), m_wingSurfaceNormalL, 1.1284160450 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, -2.7051511890), m_wingSurfaceNormalL, 0.9593401748 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, -3.1378037070), m_wingSurfaceNormalL, 0.7902643043 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, -3.5700856160), m_wingSurfaceNormalL, 0.6211884337 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, -4.0014189490), m_wingSurfaceNormalL, 0.3500000000 + 0.21931));

	//m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, 0.1067866894), m_wingSurfaceNormalR, 1.9737953980));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, 0.5399751247), m_wingSurfaceNormalR, 1.8047195280 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, 0.9731336040), m_wingSurfaceNormalR, 1.6356436570 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, 1.4062517670), m_wingSurfaceNormalR, 1.4665677860 + 0.21931));
	m_elements.push_back(AeroSurface(m_state, CLalpha, CDalpha, Vec3(0.0, 0.0, 1.8393138510), m_wingSurfaceNormalR, 1.2974919160 + 0.21931));
	//wing+ail								   
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, 2.2722946510), m_wingSurfaceNormalR, 1.1284160450 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, 2.7051511890), m_wingSurfaceNormalR, 0.9593401748 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, 3.1378037070), m_wingSurfaceNormalR, 0.7902643043 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, 3.5700856160), m_wingSurfaceNormalR, 0.6211884337 + 0.21931));
	m_elementsC.push_back(AeroControlSurface(m_state, m_airframe, CLalpha, CDalpha, Vec3(0.0, 0.0, 4.0014189490), m_wingSurfaceNormalR, 0.3500000000 + 0.21931));

}

Skyhawk::FlightModel::~FlightModel()
{

}

//Seriously need to set EVERY VARIABLE to zero (or approriate value if zero causes singularity) in the constructor and 
//in this function. Otherwise Track's become unusable because of the butterfly effect.
void Skyhawk::FlightModel::zeroInit()
{
	m_q = 0.0;
	m_p = 0.0;
	m_k = 0.0;

	m_scalarVSquared = 0.0;
	m_scalarV = 0.0;
	m_aoaPrevious = 0.0;
	m_betaPrevious = 0.0;
	m_betaDot = 0.0;

	m_LslatVel = 0.0;
	m_RslatVel = 0.0;

	m_moment = Vec3();
	m_force = Vec3();

	m_com = Vec3();
	m_airspeed = Vec3();

	m_CDwindAxesComp = Vec3();

	/*m_elementLW.zeroInit();
	m_elementRW.zeroInit();*/
	m_elementLSlat.zeroInit();
	m_elementRSlat.zeroInit();
	m_elementLFlap.zeroInit();
	m_elementRFlap.zeroInit();
	m_elementLSpoiler.zeroInit();
	m_elementRSpoiler.zeroInit();

}

void Skyhawk::FlightModel::coldInit()
{
	zeroInit();
}

void Skyhawk::FlightModel::hotInit()
{
	zeroInit();
}

void Skyhawk::FlightModel::airborneInit()
{
	zeroInit();
}

void Skyhawk::FlightModel::calculateAero(double dt)
{
	calculateLocalPhysicsParams(dt);
	slats(dt);
	lift();
	drag();
	calculateElements();
	sideForce();
	thrustForce();

	L_stab();
	M_stab();
	N_stab();

	//printf("roll rate: %lf\n", m_state.getOmega().x);
	
	calculateShake();
}

void Skyhawk::FlightModel::calculateLocalPhysicsParams(double dt)
{
	m_force = Vec3();
	m_moment = Vec3();

	double dAOA = m_state.getAOA() - m_aoaPrevious;
	//printf( "%lf - %lf = %lf\n", m_state.getAOA(), m_aoaPrevious, dAOA );
	double dAOAOpposite;

	if (dAOA >= 0.0)
	{
		dAOAOpposite = 2.0 * PI - dAOA;
	}
	else
	{
		dAOAOpposite = -2.0 * PI - dAOA;
	}

	if (fabs(dAOA) > fabs(dAOAOpposite))
	{
		//printf( "Took opposite: %lf, instead of %lf\n", dAOAOpposite, dAOA );
		dAOA = dAOAOpposite;
		
	}

	double dBeta = m_state.getBeta() - m_betaPrevious;
	double dBetaOpposite;

	if (dBeta >= 0.0)
	{
		dBetaOpposite = 2.0 * PI - dBeta;
	}
	else
	{
		dBetaOpposite = -2.0 * PI - dBeta;
	}

	if (fabs(dBeta) > fabs(dBetaOpposite))
	{
		dBeta = dBetaOpposite;
		//printf( "Took opposite: %lf\n", dBeta );
	}

	//Clamped to prevent strange behaviour from the lack of precision in the timestep.
	m_aoaDot = clamp(dAOA / dt, -10.0 * PI, 10.0 * PI);

	//Nose compression + Lower than 50 kts.
	if ( m_airframe.getNoseCompression() > 0.05 && m_scalarV < 25.0 )
	{
		m_aoaDot = 0.0;
	}

	m_betaDot = clamp( dBeta / dt, -10 * PI, 10.0 * PI );

	//Get airspeed and scalar speed squared.
	m_airspeed = m_state.getWorldVelocity() - m_state.getWorldWindVelocity();

	m_scalarVSquared = m_airspeed.x * m_airspeed.x + m_airspeed.y * m_airspeed.y + m_airspeed.z * m_airspeed.z;

	m_scalarV = sqrt(m_scalarVSquared);
	m_engine.setAirspeed(m_scalarV);
	m_interface.setAirspeed(m_scalarV);
	m_state.setMach(m_scalarV / m_state.getSpeedOfSound());

	m_aoaPrevious = m_state.getAOA();
	m_betaPrevious = m_state.getBeta();

	m_k = m_scalarVSquared * m_state.getAirDensity() * 0.5 * m_totalWingArea;
	m_q = m_k * m_totalWingSpan;
	m_p = m_scalarV * m_state.getAirDensity() * 0.25 * m_totalWingArea * m_totalWingSpan * m_totalWingSpan;
}

void Skyhawk::FlightModel::lift()
{
	addForce(Vec3(0.0, m_k * (CLde(m_state.getMach()) * elevator()), 0.0));
}

void Skyhawk::FlightModel::drag()
{
	double CD = dCDspeedBrake(0.0) * m_airframe.getSpeedBrakePosition() +
		CDbeta(m_state.getBeta()) +
		CDde(0.0) * abs(elevator()) +
		CDmach(m_state.getMach()) + CDi(0.0) * pow(CLalpha(m_state.getAOA()), 2.0) +
		m_airframe.getGearLPosition() * CDMainGear +
		m_airframe.getGearRPosition() * CDMainGear +
		m_airframe.getGearNPosition() * CDNoseGear;


	m_CDwindAxesComp.y = 0;
	m_CDwindAxesComp.z = 0;
	m_CDwindAxesComp.x = -m_k * CD;
}

void Skyhawk::FlightModel::calculateElements()
{
	Vec3 dragElem = windAxisToBody(m_CDwindAxesComp, m_state.getAOA(), m_state.getBeta() );
	

	m_elementLSlat.setLDFactor(m_airframe.getSlatLPosition(), m_airframe.getSlatLPosition());
	m_elementRSlat.setLDFactor(m_airframe.getSlatRPosition(), m_airframe.getSlatRPosition());
	m_elementLFlap.setLDFactor(m_airframe.getFlapsPosition() * m_airframe.getFlapDamage(), m_airframe.getFlapsPosition());
	m_elementRFlap.setLDFactor(m_airframe.getFlapsPosition() * m_airframe.getFlapDamage(), m_airframe.getFlapsPosition());
	m_elementLSpoiler.setLDFactor(m_airframe.getSpoilerPosition() * m_airframe.getSpoilerDamage(), m_airframe.getSpoilerPosition());
	m_elementRSpoiler.setLDFactor(m_airframe.getSpoilerPosition() * m_airframe.getSpoilerDamage(), m_airframe.getSpoilerPosition());
	m_elementHorizontalStab.setLDFactor( 0.25 * m_airframe.getHoriStabDamage(), 0.85 * m_airframe.getHoriStabDamage());
	m_elementVerticalStab.setLDFactor( 0.35 * m_airframe.getVertStabDamage(), 0.85 * m_airframe.getVertStabDamage());
	//printf("beta: %lf, aoa: %lf\n", toDegrees(m_elementVerticalStab.getAOA()));

	/*m_elementLAil.setLDFactor(m_airframe.getAileron() * m_airframe.getAileronDamage(), m_airframe.getAileron() * 0.05 * m_airframe.getAileronDamage());
	m_elementRAil.setLDFactor(-m_airframe.getAileron() * m_airframe.getAileronDamage(), -m_airframe.getAileron() * 0.05 * m_airframe.getAileronDamage());*/

	m_elementLSlat.calculateElementPhysics();
	m_elementRSlat.calculateElementPhysics();
	m_elementLFlap.calculateElementPhysics();
	m_elementRFlap.calculateElementPhysics();
	m_elementLSpoiler.calculateElementPhysics();
	m_elementRSpoiler.calculateElementPhysics();
	//m_elementLAil.calculateElementPhysics();
	//m_elementRAil.calculateElementPhysics();
	m_elementHorizontalStab.calculateElementPhysics();
	m_elementVerticalStab.calculateElementPhysics();

	//printf("aoa hstab: %lf\n", toDegrees(m_elementHorizontalStab.m_aoa));
	//printf("forceL: %lf, %lf, %lf  forceR: %lf, %lf, %lf\n", m_elementLAil.getForce().x, m_elementLAil.getForce().y, m_elementLAil.getForce().z, m_elementRAil.getForce().x, m_elementRAil.getForce().y, m_elementRAil.getForce().z);

	addForceElement(m_elementLSlat);
	addForceElement(m_elementRSlat);
	addForceElement(m_elementLFlap);
	addForceElement(m_elementRFlap);
	addForceElement(m_elementLSpoiler);
	addForceElement(m_elementRSpoiler);

	//m_moment += m_elementLAil.getMoment();
	//m_moment += m_elementRAil.getMoment();
	m_moment += m_elementHorizontalStab.getMoment();
	m_moment += m_elementVerticalStab.getMoment();
	
	//addForceElement(m_elementHorizontalStab);
	//addForceElement(m_elementVerticalStab);

	// i:0-8 = LW, i:8-17 = RW
	float damage = 1.0;

	for (int i = 0; i < m_elements.size(); i++)
	{
		if (i < m_elements.size() / 2)
		{
			damage = m_airframe.getLWingDamage();
		}
		else
		{
			damage = m_airframe.getRWingDamage();
		}
		m_elements[i].setLDFactor(1.0 * damage, 0.7 * damage);
		m_elements[i].calculateElementPhysics();
		addForceElement(m_elements[i]);
		//printf("id: %d, aoa: %lf, beta: %lf\n", i, toDegrees(m_elements[i].getAOA()), toDegrees(m_elements[i].getBeta()));
	}

	for (int i = 0; i < m_elementsC.size(); i++)
	{
		if (i < m_elementsC.size() / 2)
		{
			damage = m_airframe.getLWingDamage();
		}
		else
		{
			damage = m_airframe.getRWingDamage();
		}
		m_elementsC[i].setLDFactor(1.0 * damage, 0.7 * damage);
		m_elementsC[i].calculateElementPhysics();
		addForceElement(m_elementsC[i]);
		//printf("id: %d, aoa: %lf, beta: %lf\n", i, toDegrees(m_elements[i].getAOA()), toDegrees(m_elements[i].getBeta()));
	}

	//printf("[HSTAB] aoa: %lf, beta: %lf, moment: %lf, %lf, %lf\n", toDegrees(m_elementHorizontalStab.getAOA()), toDegrees(m_elementHorizontalStab.getBeta()), m_elementHorizontalStab.getMoment().x, m_elementHorizontalStab.getMoment().y, m_elementHorizontalStab.getMoment().z);
	//printf("[VSTAB] aoa: %lf, beta: %lf, moment: %lf, %lf, %lf\n", toDegrees(m_elementVerticalStab.getAOA()), toDegrees(m_elementVerticalStab.getBeta()), m_elementVerticalStab.getMoment().x, m_elementVerticalStab.getMoment().y, m_elementVerticalStab.getMoment().z);

	addForce(dragElem);
}

void Skyhawk::FlightModel::slats(double& dt)
{
	double forceL = (m_elementLSlat.m_kElem / m_totalWingArea) * m_slatArea * slatCL(m_elementLSlat.getAOA());
	double forceR = (m_elementLSlat.m_kElem / m_totalWingArea) * m_slatArea * slatCL(m_elementRSlat.getAOA());

	Vec3 accDirection(-0.94, 0.34, 0.0);
	double accAircraft = normalize(accDirection) * m_state.getLocalAcceleration();

	double x_L = m_airframe.getSlatLPosition();
	double x_R = m_airframe.getSlatRPosition();

	double a_L = accAircraft + (0.09 * forceL - m_slatDamping * m_LslatVel - 0.09 * m_slatSpring * (x_L-1.5) ) / m_slatMass;
	double a_R = accAircraft + (0.09 * forceR - m_slatDamping * m_RslatVel - 0.09 * m_slatSpring * (x_R-1.5) ) / m_slatMass;

	m_LslatVel += a_L * dt;
	m_RslatVel += a_R * dt;

	x_L += m_LslatVel * dt;
	x_R += m_RslatVel * dt;
	
	if ( x_L < 0.0 )
	{
		x_L = 0.0;
		m_LslatVel = -m_LslatVel*0.3;
	}
	else if ( x_L > 1.0 )
	{
		x_L = 1.0;
		m_LslatVel = -m_LslatVel*0.3;
	}

	if ( x_R < 0.0 )
	{
		x_R = 0.0;
		m_RslatVel = -m_RslatVel*0.3;
	}
	else if ( x_R > 1.0 )
	{
		x_R = 1.0;
		m_RslatVel = -m_RslatVel*0.3;
	}

	m_airframe.setSlatLPosition( x_L );
	m_airframe.setSlatRPosition( x_R );
}

void Skyhawk::FlightModel::calculateShake()
{
	double shakeAmplitude{ 0.0 };
	double buffetAmplitude = 0.6 * m_cockpitShakeModifier;
	double x{ 0.0 };

	// 19 - 28
	double aoa = std::abs(m_state.getAOA());
	if (aoa >= 0.332 && m_scalarV > 30.0)
	{
		x = std::min(aoa - 0.332, 0.157)/0.157;
		shakeAmplitude += x * buffetAmplitude;
	}

	m_cockpitShake = shakeAmplitude;
}

void Skyhawk::FlightModel::csvData(std::vector<double>& data)
{
	

	
}