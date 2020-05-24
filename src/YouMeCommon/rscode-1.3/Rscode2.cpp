#include "Rscode2.h"
#include <string.h>
#include <stdio.h>
#include "RscodeDefine.h"

#define RSCODE_DEBUG 0

#include <iostream>
using namespace std;
namespace youmecommon {
	int CRscode2::gexp[512] = { 0 };
	int CRscode2::glog[256] = { 0 };
    bool CRscode2::isGaloisInited = false ;

	void CRscode2::Modified_Berlekamp_Massey(void)
	{
		int n, L, L2, k, d, i;

        int* psi = new int[MAXDEG];
        int* psi2 = new int[MAXDEG];
        int* D = new int[MAXDEG];
        int* gamma = new int[MAXDEG];

		/* initialize Gamma, the erasure locator polynomial */
		init_gamma(gamma);

		/* initialize to z */
		copy_poly(D, gamma);
		mul_z_poly(D);

		copy_poly(psi, gamma);
		k = -1; L = NErasures;

		for (n = NErasures; n < NPAR; n++) {

			d = compute_discrepancy(psi, synBytes, L, n);

			if (d != 0) {

				/* psi2 = psi - d*D */
				for (i = 0; i < MAXDEG; i++) psi2[i] = psi[i] ^ gmult(d, D[i]);


				if (L < (n - k)) {
					L2 = n - k;
					k = n - L;
					/* D = scale_poly(ginv(d), psi); */
					for (i = 0; i < MAXDEG; i++) D[i] = gmult(psi[i], ginv(d));
					L = L2;
				}

				/* psi = psi2 */
				for (i = 0; i < MAXDEG; i++) psi[i] = psi2[i];
			}

			mul_z_poly(D);
		}

		for (i = 0; i < MAXDEG; i++) Lambda[i] = psi[i];
		compute_modified_omega();
        
        delete [] psi;
        delete [] psi2;
        delete [] D;
        delete [] gamma;
	}


	void CRscode2::init_gamma(int gamma[])
	{
		int e;
        
        int* tmp = new int[MAXDEG];

		zero_poly(gamma);
		zero_poly(tmp);
		gamma[0] = 1;

		for (e = 0; e < NErasures; e++) {
			copy_poly(tmp, gamma);
			scale_poly(gexp[ErasureLocs[e]], tmp);
			mul_z_poly(tmp);
			add_polys(gamma, tmp);
		}
        
        delete []tmp;
	}

	void CRscode2::add_polys(int dst[], int src[])
	{
		int i;
		for (i = 0; i < MAXDEG; i++) dst[i] ^= src[i];
	}

	void CRscode2::scale_poly(int k, int poly[])
	{
		int i;
		for (i = 0; i < MAXDEG; i++) poly[i] = gmult(k, poly[i]);
	}

	void CRscode2::mul_z_poly(int src[])
	{
		int i;
		for (i = MAXDEG - 1; i > 0; i--) src[i] = src[i - 1];
		src[0] = 0;
	}


	int CRscode2::compute_discrepancy(int lambda[], int S[], int L, int n)
	{
		int i, sum = 0;

		for (i = 0; i <= L; i++)
			sum ^= gmult(lambda[i], S[n - i]);
		return (sum);
	}

	void CRscode2::compute_modified_omega()
	{
		int i;
        
        int* product = new int[MAXDEG*2];

		mult_polys(product, Lambda, synBytes);
		zero_poly(Omega);
		for (i = 0; i < NPAR; i++) Omega[i] = product[i];
        
        delete [] product;

	}

	void CRscode2::Find_Roots(void)
	{
		int sum, r, k;
		NErrors = 0;

		for (r = 1; r < 256; r++) {
			sum = 0;
			/* evaluate lambda at r */
			for (k = 0; k < NPAR + 1; k++) {
				sum ^= gmult(gexp[(k*r) % 255], Lambda[k]);
			}
			if (sum == 0)
			{
				ErrorLocs[NErrors] = (255 - r); NErrors++;
				if (RSCODE_DEBUG) fprintf(stderr, "Root found at r = %d, (255-r) = %d\n", r, (255 - r));
			}
		}
	}

	void CRscode2::build_codeword(unsigned char msg[], int nbytes, unsigned char dst[])
	{
		int i;

		for (i = 0; i < nbytes; i++) dst[i] = msg[i];

		for (i = 0; i < NPAR; i++) {
			dst[i + nbytes] = pBytes[NPAR - 1 - i];
		}
	}

	void CRscode2::init_exp_table(void)
	{
		int i, z;
		int pinit, p1, p2, p3, p4, p5, p6, p7, p8;

		pinit = p2 = p3 = p4 = p5 = p6 = p7 = p8 = 0;
		p1 = 1;

		gexp[0] = 1;
		gexp[255] = gexp[0];
		glog[0] = 0;			/* shouldn't log[0] be an error? */

		for (i = 1; i < 256; i++) {
			pinit = p8;
			p8 = p7;
			p7 = p6;
			p6 = p5;
			p5 = p4 ^ pinit;
			p4 = p3 ^ pinit;
			p3 = p2 ^ pinit;
			p2 = p1;
			p1 = pinit;
			gexp[i] = p1 + p2 * 2 + p3 * 4 + p4 * 8 + p5 * 16 + p6 * 32 + p7 * 64 + p8 * 128;
			gexp[i + 255] = gexp[i];
		}

		for (i = 1; i < 256; i++) {
			for (z = 0; z < 256; z++) {
				if (gexp[z] == i) {
					glog[i] = z;
					break;
				}
			}
		}
	}

	void CRscode2::mult_polys(int dst[], int p1[], int p2[])
	{
		int i, j;
        int* tmp1 = new int[MAXDEG*2];

		for (i = 0; i < (MAXDEG * 2); i++) dst[i] = 0;

		for (i = 0; i < MAXDEG; i++) {
			for (j = MAXDEG; j < (MAXDEG * 2); j++) tmp1[j] = 0;

			/* scale tmp1 by p1[i] */
			for (j = 0; j < MAXDEG; j++) tmp1[j] = gmult(p2[j], p1[i]);
			/* and mult (shift) tmp1 right by i */
			for (j = (MAXDEG * 2) - 1; j >= i; j--) tmp1[j] = tmp1[j - i];
			for (j = 0; j < i; j++) tmp1[j] = 0;

			/* add into partial product */
			for (j = 0; j < (MAXDEG * 2); j++) dst[j] ^= tmp1[j];
		}
        
        delete[] tmp1;
	}

	void CRscode2::compute_genpoly(int nbytes, int genpoly[])
	{
		int i, tp[256], tp1[256];

		/* multiply (x + a^n) for n = 1 to nbytes */

		zero_poly(tp1);
		tp1[0] = 1;

		for (i = 1; i <= nbytes; i++) {
			zero_poly(tp);
			tp[0] = gexp[i];		/* set up x+a^n */
			tp[1] = 1;

			mult_polys(genpoly, tp, tp1);
			copy_poly(tp1, genpoly);
		}
	}

	/* generator polynomial */
    CRscode2::CRscode2( int npar ):NPAR( npar ),
    MAXDEG( 2*npar )
	{

		/* Decoder syndrome bytes */
        synBytes = new int[MAXDEG];
		memset(synBytes, 0, MAXDEG * sizeof(int));

		/* The Error Locator Polynomial, also known as Lambda or Sigma. Lambda[0] == 1 */
        Lambda = new int[MAXDEG];
		memset(Lambda, 0, MAXDEG * sizeof(int));
		/* The Error Evaluator Polynomial */
        Omega = new int[MAXDEG];
		memset(Omega, 0, MAXDEG * sizeof(int));

		/* error locations found using Chien's search*/
		memset(ErrorLocs, 0, sizeof(ErrorLocs));
		NErrors = 0;

		/* erasure flags */
		memset(ErasureLocs, 0, sizeof(ErasureLocs));
		NErasures = 0;


		/* Encoder parity bytes */
        pBytes = new int[MAXDEG];
		memset(pBytes, 0, MAXDEG * sizeof(int));
        
        genPoly = new int[MAXDEG * 2 ];
        memset(genPoly, 0, MAXDEG * 2 * sizeof(int));
        
        if( isGaloisInited == false ){
            initialize_ecc();
        }
        
        /* Compute the encoder generator polynomial */
        compute_genpoly(NPAR, genPoly);

	}


	CRscode2::~CRscode2()
	{
        delete[] synBytes;
        delete[] Lambda;
        delete[] Omega;
        delete[] pBytes;
        delete[] genPoly;
	}

	void CRscode2::initialize_ecc()
	{
        isGaloisInited = true;
 		/* Initialize the galois field arithmetic tables */
		init_galois_tables();
	}
    
    int CRscode2::getNPAR(){
        return NPAR;
    }

	void CRscode2::encode_data(unsigned char msg[], int nbytes, unsigned char dst[])
	{
		int i,  dbyte, j;
        
        int* LFSR = new int[NPAR + 1];

		for (i = 0; i < NPAR + 1; i++) LFSR[i] = 0;

		for (i = 0; i < nbytes; i++) {
			dbyte = msg[i] ^ LFSR[NPAR - 1];
			for (j = NPAR - 1; j > 0; j--) {
				LFSR[j] = LFSR[j - 1] ^ gmult(genPoly[j], dbyte);
			}
			LFSR[0] = gmult(genPoly[0], dbyte);
		}

		for (i = 0; i < NPAR; i++)
			pBytes[i] = LFSR[i];

		build_codeword(msg, nbytes, dst);
        
        delete [] LFSR;
	}

	void CRscode2::decode_data(unsigned char data[], int nbytes)
	{
		int i, j, sum;
		for (j = 0; j < NPAR; j++) {
			sum = 0;
			for (i = 0; i < nbytes; i++) {
				sum = data[i] ^ gmult(gexp[j + 1], sum);
			}
			synBytes[j] = sum;
		}
	}

	int CRscode2::check_syndrome(void)
	{
		int i, nz = 0;
		for (i = 0; i < NPAR; i++) {
			if (synBytes[i] != 0) {
				nz = 1;
				break;
			}
		}
		return nz;
	}

	int CRscode2::correct_errors_erasures(unsigned char codeword[], int csize, int nerasures, int erasures[])
	{
		int r, i, j, err;

		/* If you want to take advantage of erasure correction, be sure to
		set NErasures and ErasureLocs[] with the locations of erasures.
		*/
		NErasures = nerasures;
		for (i = 0; i < NErasures; i++) ErasureLocs[i] = erasures[i];

		Modified_Berlekamp_Massey();
		Find_Roots();


		if ((NErrors <= NPAR) && NErrors > 0) {

			/* first check for illegal error locs */
			for (r = 0; r < NErrors; r++) {
				if (ErrorLocs[r] >= csize) {
					if (RSCODE_DEBUG) fprintf(stderr, "Error loc i=%d outside of codeword length %d\n", i, csize);
					return(0);
				}
			}

			for (r = 0; r < NErrors; r++) {
				int num, denom;
				i = ErrorLocs[r];
				/* evaluate Omega at alpha^(-i) */

				num = 0;
				for (j = 0; j < MAXDEG; j++)
					num ^= gmult(Omega[j], gexp[((255 - i)*j) % 255]);

				/* evaluate Lambda' (derivative) at alpha^(-i) ; all odd powers disappear */
				denom = 0;
				for (j = 1; j < MAXDEG; j += 2) {
					denom ^= gmult(Lambda[j], gexp[((255 - i)*(j - 1)) % 255]);
				}

				err = gmult(num, ginv(denom));
				if (RSCODE_DEBUG) fprintf(stderr, "Error magnitude %#x at loc %d\n", err, csize - i);

				codeword[csize - i - 1] ^= err;
			}
			return(1);
		}
		else {
			if (RSCODE_DEBUG && NErrors) fprintf(stderr, "Uncorrectable codeword\n");
			return(0);
		}
	}
}
