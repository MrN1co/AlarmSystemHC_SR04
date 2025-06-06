#ifndef KLAW_H
#define KLAW_H

#include "MKL05Z4.h"
#define S1	6						// Numer bitu dla klawisza S1
#define S2	7						// Numer bitu dla klawisza S2
#define S3	11						// Numer bitu dla klawisza S3
#define S4	12						// Numer bitu dla klawisza S4
#define S1_MASK	(1<<S1)		// Maska dla klawisza S1
#define S2_MASK	(1<<S2)		// Maska dla klawisza S2
#define S3_MASK	(1<<S3)		// Maska dla klawisza S3
#define S4_MASK	(1<<S4)		// Maska dla klawisza S4


void Klaw_Init(void);
void Klaw_S2_4_Int(void);

#endif  /* KLAW_H */
