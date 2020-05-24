#pragma once
#include "RscodeDefine.h"
namespace youmecommon {
#define NPAR 1
#define MAXDEG (2*NPAR)
#define DEBUG 0
    
	class CRscode
	{
	public:
		CRscode();
		~CRscode();


	public:
		static void initialize_ecc();
		void encode_data(unsigned char msg[], int nbytes, unsigned char dst[]);
		void decode_data(unsigned char data[], int nbytes);

		/* Check if the syndrome is zero */
		int check_syndrome(void);


		int correct_errors_erasures(unsigned char codeword[],
			int csize,
			int nerasures,
			int erasures[]);

	private:

		void Modified_Berlekamp_Massey(void);


		/* gamma = product (1-z*a^Ij) for erasure locs Ij */
		void init_gamma(int gamma[]);

		void add_polys(int dst[], int src[]);


		void scale_poly(int k, int poly[]);


		/* multiply by z, i.e., shift right by 1 */
		void mul_z_poly(int src[]);

		int
			compute_discrepancy(int lambda[], int S[], int L, int n);


		void
			compute_modified_omega();


		void
			Find_Roots(void);

		int ginv(int elt)
		{
			return (gexp[255 - glog[elt]]);
		}

		void
			build_codeword(unsigned char msg[], int nbytes, unsigned char dst[]);

		void
			static	init_exp_table(void);

		static void init_galois_tables(void)
		{
			/* initialize the table of powers of alpha */
			init_exp_table();
		}
		static void zero_poly(int poly[])
		{
			int i;
			for (i = 0; i < MAXDEG; i++) poly[i] = 0;
		}

		/* multiplication using logarithms */
		static int gmult(int a, int b)
		{
			int i, j;
			if (a == 0 || b == 0) return (0);
			i = glog[a];
			j = glog[b];
			return (gexp[i + j]);
		}
		static void mult_polys(int dst[], int p1[], int p2[]);

		static void compute_genpoly(int nbytes, int genpoly[]);


		static void copy_poly(int dst[], int src[])
		{
			int i;
			for (i = 0; i < MAXDEG; i++) dst[i] = src[i];
		}
	private:

		/* Decoder syndrome bytes */
		int synBytes[MAXDEG];


		/* The Error Locator Polynomial, also known as Lambda or Sigma. Lambda[0] == 1 */
		int Lambda[MAXDEG];

		/* The Error Evaluator Polynomial */
		int Omega[MAXDEG];


		/* error locations found using Chien's search*/
		int ErrorLocs[256];
		int NErrors;

		/* erasure flags */
		int ErasureLocs[256];
		int NErasures;


		/* Encoder parity bytes */
		int pBytes[MAXDEG];


		/* generator polynomial */
		static int genPoly[MAXDEG * 2];


		static int gexp[512];
		static int glog[256];
	};

}
