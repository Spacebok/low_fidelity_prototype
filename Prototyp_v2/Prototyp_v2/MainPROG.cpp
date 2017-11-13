#define _USE_MATH_DEFINES

#include "Definitions.h"
#include <iostream>
#include <thread>
#include <math.h>

typedef struct EPOSdata {
	HANDLE epos;
	WORD NodeId;
	DWORD pErrorCode;
	long initPos;
}EPOS;

// Variablen und Parameter
EPOS EPOS1;
EPOS EPOS2;
int temp;
char tempchar[10];
double tempd;
long positionMust = 0;
long positionIs = 0;
bool ok = true;
bool stateRegler = false;
bool newPosition = false;
// Regelparameter
double Kp = 15000;
double Kd = 300;

void threadRegler() {
	std::cout << "Regler gestartet\n";
	long angle127;
	long angle1;
	double deltaAngle127;
	double deltaAngle1;
	double I127;
	double I1;
	short I127Must;
	short I1Must;
	long vIs127;
	long vIs1;
	double vIs127d;
	double vIs1d;

	while (true) {
		// ********************************************************
		// Hier muss der Regler implementiert werden
		// ********************************************************

		VCS_GetPositionIs(EPOS1.epos, EPOS1.NodeId, &angle127, &EPOS1.pErrorCode);
		VCS_GetPositionIs(EPOS2.epos, EPOS2.NodeId, &angle1, &EPOS1.pErrorCode);
		VCS_GetVelocityIs(EPOS1.epos, EPOS1.NodeId, &vIs127, &EPOS1.pErrorCode);
		VCS_GetVelocityIs(EPOS2.epos, EPOS2.NodeId, &vIs1, &EPOS1.pErrorCode);
		vIs127d = double(vIs127) / 60 * 2 * M_PI;
		vIs1d = double(vIs1) / 60 * 2 * M_PI;
		deltaAngle127 = (double(EPOS1.initPos) - double(angle127)) / 25600 * 2 * M_PI;
		deltaAngle1 = (double(EPOS2.initPos) - double(angle1)) / 25600 * 2 * M_PI;
		I127 = Kp*deltaAngle127 - Kd*vIs127d;
		I1 = Kp*deltaAngle1 - Kd*vIs1d;
		I127Must = short(I127);
		I1Must = short(I1);

		VCS_SetCurrentMust(EPOS1.epos, EPOS1.NodeId, I127Must, &EPOS1.pErrorCode);
		VCS_SetCurrentMust(EPOS2.epos, EPOS2.NodeId, I1Must, &EPOS2.pErrorCode);

		if (!stateRegler) {
			break;
		}
		// ********************************************************
		// ********************************************************
	}
}


int main(void) {

	std::cout << "Low Level Controller v1\n*****************************************\n";

	// *******************************************************************
	// Verbindung zu Controller 1
	EPOS1.NodeId = 127;
	EPOS1.epos = VCS_OpenDeviceDlg(&EPOS1.pErrorCode);
	// Clear Fault
	VCS_ClearFault(EPOS1.epos, EPOS1.NodeId, &EPOS1.pErrorCode);
	// Set Current Mode
	std::cout << "Set Current Mode EPOS 1\n";
	if (!VCS_ActivateCurrentMode(EPOS1.epos, EPOS1.NodeId, &EPOS1.pErrorCode)) {
		std::cout << "Fehler beim setzen des Current Modes EPOS1!\n";
	}
	// Verbindung zu Controller 2
	EPOS2.NodeId = 1;
	EPOS2.epos = VCS_OpenDeviceDlg(&EPOS2.pErrorCode);
	// Clear Fault
	VCS_ClearFault(EPOS2.epos, EPOS2.NodeId, &EPOS2.pErrorCode);
	// Set Current Mode
	std::cout << "Set Current Mode EPOS 2\n";
	if (!VCS_ActivateCurrentMode(EPOS2.epos, EPOS2.NodeId, &EPOS2.pErrorCode)) {
		std::cout << "Fehler beim setzen des Current Modes EPOS2!\n";
	}
	// *******************************************************************

	// Enable Devices
	std::cout << "Enable Device 1\n";
	if (!VCS_SetEnableState(EPOS1.epos, EPOS1.NodeId, &EPOS1.pErrorCode)) {
		std::cout << "Fehler beim aktivieren der Steuerung 1!\n";
	}
	std::cout << "Enable Device 2\n";
	if (!VCS_SetEnableState(EPOS2.epos, EPOS2.NodeId, &EPOS2.pErrorCode)) {
		std::cout << "Fehler beim aktivieren der Steuerung 2!\n";
	}
	// Initialisierung
	// EPOS 1
	if (!VCS_GetPositionIs(EPOS1.epos, EPOS1.NodeId, &EPOS1.initPos, &EPOS1.pErrorCode)) {
		std::cout << "Fehler beim lesen der aktuellen Position!\n";
	}
	// EPOS 2
	if (!VCS_GetPositionIs(EPOS2.epos, EPOS2.NodeId, &EPOS2.initPos, &EPOS1.pErrorCode)) {
		std::cout << "Fehler beim lesen der aktuellen Position!\n";
	}
	std::cout << "EPOS1: " << EPOS1.initPos << "\n";
	std::cout << "EPOS2: " << EPOS2.initPos << "\n";
	std::cout << "Beliebige Taste und Enter druecken um den Regler zu starten.....\n";
	std::cin >> tempchar;

	// Starte Regler in neuem Thread
	stateRegler = true;
	std::thread regler(threadRegler);

	// *******************************************************************
	// Hauptschleife
	// *******************************************************************
	while (ok) {
		std::cout << "Parameter waehlen:\n1 Kp\n2 Kd\n3 Regler beenden\n";
		std::cin >> temp;
		std::cout << "\n";
		switch (temp)
		{
		case 1:
			std::cin >> tempd;
			std::cout << "\n";
			Kp = tempd;
			break;
		case 2:
			std::cin >> tempd;
			std::cout << "\n";
			Kd = tempd;
			break;
		case 3:
			stateRegler = false;
			ok = false;
			break;
		default:
			break;
		}
	}
	// *******************************************************************
	// *******************************************************************
	// Warte bis Regler beendet
	regler.join();

	// Disable Device
	std::cout << "\nDisable EPOS 1\n";
	if (!VCS_SetDisableState(EPOS1.epos, EPOS1.NodeId, &EPOS1.pErrorCode)) {
		std::cout << "Fehler beim deaktivieren der Steuerung 1!\n";
	}
	// Disable Device
	std::cout << "\nDisable EPOS 2\n";
	if (!VCS_SetDisableState(EPOS2.epos, EPOS2.NodeId, &EPOS2.pErrorCode)) {
		std::cout << "Fehler beim deaktivieren der Steuerung 2!\n";
	}

	// Close Device
	VCS_CloseDevice(EPOS1.epos, &EPOS1.pErrorCode);
	// Close Device
	VCS_CloseDevice(EPOS2.epos, &EPOS2.pErrorCode);


}