#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h> //for IOCTL defs
#include <fcntl.h>
#include <time.h>

#define I2C_FICHIER "/dev/i2c-1" // fichier Linux representant le BUS #2
#define I2C_ADRESSE 0x29 // adresse du Device I2C VL6180XV0NR/1 (Capteur de distance)
#define CAPTEURDISTANCE_TUNNING_NOMBRE 40


typedef struct
{
	uint16_t adresse;
	uint8_t valeur;
} CAPTEURDISTANCE_TUNNING;


//variables privées
 CAPTEURDISTANCE_TUNNING CapteurDistance_Tunning [CAPTEURDISTANCE_TUNNING_NOMBRE] =
{
	{0x0207, 0x01}, {0x0208, 0x01}, {0x0096, 0x00}, {0x0097, 0xfd},
	{0x00e3, 0x00}, {0x00e4, 0x04},	{0x00e5, 0x02},	{0x00e6, 0x01},
	{0x00e7, 0x03},	{0x00f5, 0x02},	{0x00d9, 0x05},	{0x00db ,0xce},
	{0x00dc, 0x03},	{0x00dd, 0xf8},	{0x009f, 0x00},	{0x00a3, 0x3c},
	{0x00b7, 0x00},	{0x00bb, 0x3c},	{0x00b2, 0x09},	{0x00ca, 0x09},
	{0x0198, 0x01},	{0x01b0, 0x17},	{0x01ad, 0x00},	{0x00ff, 0x05},
	{0x0100, 0x05},	{0x0199, 0x05},	{0x01a6, 0x1b},	{0x01ac, 0x3e},
	{0x01a7, 0x1f},	{0x0030, 0x00},
	{0x0011, 0x10},// Enables polling for ‘New Sample ready’ when measurement completes
	{0x010a, 0x30},// Set the averaging sample period (compromise between lower noise and increased execution time)
	{0x003f, 0x46},// Sets the light and dark gain (upper nibble). Dark gain should not be changed.
	{0x0031, 0xFF},// sets the # of range measurements after which auto calibration of system is performed 
	{0x0040, 0x63},// Set ALS integration time to 100ms
	{0x002e, 0x01},// perform a single temperature calibration of the ranging sensor 
	{0x001b, 0x09},// *Set default ranging inter-measurement period to 100ms
	{0x003e, 0x31},// *Set default ALS inter-measurement period to 500ms
	{0x0014, 0x24},// *Configures interrupt on ‘New Sample Ready threshold event’
	{0x0016, 0x00} //*change fresh out of set status to 0
};

int fdPortI2C;  // file descriptor I2C

int InitialiseCapteurDistance(void);
int Lecture_Distance(float *Distance);
int Ecriture_I2C_Capteur(uint16_t Registre, uint8_t Donnee);
int Lecture_I2C_Capteur(uint16_t Registre, uint8_t *Donnee);

int main()
{
	int NombreLecture = 0;
	float Donnee;
	// Initialisation du port I2C, 
	if(InitialiseCapteurDistance() < 0)
	{
		printf("erreur initialisation main\n");
	}

	while(NombreLecture != 200)
	{
		NombreLecture++;
		usleep(100000);
		if(Lecture_Distance(&Donnee) < 0)
		{
			printf("erreur lecture distance main\n");
		}
		else 
		{
			printf("Distance: %0.02f cm\n", Donnee);
		}
	}
	
	
	close(fdPortI2C); /// Fermeture du 'file descriptor'
	return 0;
}


/* A REVOIR */

int Lecture_Distance(float *Distance)
{
uint8_t valeur;

	if (Ecriture_I2C_Capteur(0x18, 0x01) < 0)
	{
		printf("erreur: Ecriture_I2C_Capteur 1\n");
		return -1;
	}
	if(Lecture_I2C_Capteur(0x4F, &valeur) < 0)
	{
		printf("erreur: Lecture_Distance 2\n");
		return -1;
	}

	while((valeur & 0x07) != 0x04)
  	{
    	if (Lecture_I2C_Capteur(0x4F, &valeur) < 0)
    	{
    		printf("erreur: Lecture_Distance 3\n");
      		return -1;
    	}
  	}

  	if (Lecture_I2C_Capteur(0x62, &valeur) < 0)
  	{
		printf("erreur: Lecture_Distance 4\n");
    	return -1;
	}
	if (Ecriture_I2C_Capteur(0x15, 0x07) < 0)
 	{
  		printf("erreur: Lecture_Distance 5\n");
    	return -1;
	}
	*Distance = (float)valeur /10.0;
	return 0;
}


int InitialiseCapteurDistance(void)
{
uint8_t index;

	fdPortI2C = open(I2C_FICHIER, O_RDWR); // ouverture du 'fichier', création d'un 'file descriptor' vers le port I2C
	if(fdPortI2C == -1)
	{
		printf("erreur: I2C initialisation step 1\n");
		return -1;
	}
	if(ioctl(fdPortI2C, I2C_SLAVE_FORCE, I2C_ADRESSE) < 0)  // I2C_SLAVE_FORCE if it is already in use by a driver (i2cdetect : UU)
	{
		printf("erreur: I2C initialisation step 2\n");
		close(fdPortI2C);
		return -1;
	}


	for (index = 0; index < CAPTEURDISTANCE_TUNNING_NOMBRE; index++)
	{
		if (Ecriture_I2C_Capteur(CapteurDistance_Tunning[index].adresse, 
				CapteurDistance_Tunning[index].valeur) < 0)
		{
			printf("erreur: initialisation Capteur Distance - index %d\n", index);
			return -1;
		}
	}
}


int Ecriture_I2C_Capteur(uint16_t Registre, uint8_t Donnee)
{
uint8_t message[3];
	message[0] = (uint8_t)(Registre >> 8);
	message[1] = (uint8_t)(Registre & 0xFF);
	message[2] = Donnee;
	if(write(fdPortI2C, message, 3) != 3)
	{
		printf("erreur: Écriture I2C\n");
		close(fdPortI2C);
		return -1;
	}
	return 0;
}

int Lecture_I2C_Capteur(uint16_t Registre, uint8_t *Donnee)
{
uint8_t Commande[2];
	Commande[0] = (uint8_t)(Registre >> 8);
	Commande[1] = (uint8_t)(Registre & 0xFF);

	if(write(fdPortI2C, Commande, 2) != 2)
	{
		printf("erreur: Écriture I2C\n");
		close(fdPortI2C);
		return -1;
	}
	if (read(fdPortI2C, Donnee, 1) != 1)
	{
		printf("erreur: Lecture I2C\n");
		close(fdPortI2C);
		return -1;
	}
	return 0;
}


