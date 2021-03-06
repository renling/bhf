#include <iostream>
#include <sys/time.h>
#include <memory.h>
#include <osrng.h>
#include <sha.h>
#include <sha3.h>
#include <modes.h>
#include <blowfish.h>
#include <omp.h>
using namespace std;

double get_wall_time()
{
    struct timeval time;
    if ( gettimeofday(&time, NULL) )
    	return 0;
	return (double) time.tv_sec + (double) time.tv_usec * .000001;
}

double hash_test(int batchSize, int totalSize, int numThreads)
{
	int N = batchSize;
	long long M = ((long long) totalSize) * 1024 * 1024 * 1024 / N;
	CryptoPP::AutoSeededRandomPool rng;
	byte* message = new byte [N];
	rng.GenerateBlock(message, N);

	CryptoPP::SHA256 hash[8];

	double start = get_wall_time();
	#pragma omp parallel
	#pragma omp for
	for (int k = 0; k < numThreads; k++)
	{
		#pragma unroll
		for (int i = 0; i < M; i++)
		{
			hash[k].Update(message, N);
		}
	}
	double end = get_wall_time();
	return end - start;
}

double aes_test(int batchSize, int totalSize, int numThreads)
{
	int N = batchSize;
	long long M = ((long long) totalSize) * 1024 * 1024 * 1024 / N;
	CryptoPP::AutoSeededRandomPool rng;
	const int KeyLen = CryptoPP::AES::DEFAULT_KEYLENGTH;
	byte* key = new byte [KeyLen];

	typedef CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption ECB_AES;
	ECB_AES aes[8];
	for (int k = 0; k < 8; k++)
	{
		rng.GenerateBlock(key, KeyLen);
		aes[k].SetKey(key, KeyLen);
	}


	byte* ptxt = new byte [N];
	byte* ctxt = new byte [N];
	rng.GenerateBlock(ptxt, N);

	double start = get_wall_time();
	#pragma omp parallel
	#pragma omp for
	for (int k = 0; k < numThreads; k++)
	{
		#pragma unroll
		for (int i = 0; i < M; i++)
		{
			aes[k].ProcessData(ctxt, ptxt, N);
		}
	}
	double end = get_wall_time();
	return end - start;
}

double mem_test(int batchSize, int totalSize, int numThreads)
{
	int N = batchSize;
	long long M = totalSize;
	M = M * 1024 * 1024 * 1024 / N;

	CryptoPP::AutoSeededRandomPool rng;
	byte *src[8], *dst[8]; 
	for (int k = 0; k < numThreads; k++)
	{
		src[k] = new byte [N];
		dst[k] = new byte [N];
		rng.GenerateBlock(src[k], N);
		rng.GenerateBlock(dst[k], N);
	}

	double start = get_wall_time();
	#pragma omp parallel
	#pragma omp for
	for (int k = 0; k < numThreads; k++)
		for (int i = 0; i < M; i++)
		{
			memcpy(dst[k], src[k], N);	
		}
	double end = get_wall_time();
	return end - start;
}

double arith_test(int batchSize, int totalSize, int numThreads)
{
	int N = batchSize;
	long long M = totalSize;
	M = M * 1024 * 1024 * 1024 / N;

	int tmp[32];
	double start = get_wall_time();
	#pragma omp parallel
	#pragma omp for
	for (int k = 0; k < numThreads; k++)
		for (int i = 0; i < M; i++)
		{
			#pragma unroll
			for (int j = 0; j < N / 4 / 8; j++)
			{
				tmp[i] += tmp[i];
			}
		}
	double end = get_wall_time();
	return end - start + tmp[17] * 0;
}

int main(int argc, char *argv[])
{
	int batchSize = 16; 	// in B
	int totalSize = 4;			// in GB
	int numThreads = 2;

	string test = "hash";

	if (argc >= 5)
		numThreads = atoi(argv[4]);
	if (argc >= 4)
		totalSize = atoi(argv[3]);
	if (argc >= 3)
		batchSize = atoi(argv[2]);
	if (argc >= 2)
		test = string(argv[1]);

	double elapsed_time;

	if (test == "hash")	
		elapsed_time = hash_test(batchSize, totalSize, numThreads);
	else if (test == "aes")	
		elapsed_time = aes_test(batchSize, totalSize, numThreads);
	else if (test == "mem")	
		elapsed_time = mem_test(batchSize, totalSize, numThreads);
	else if (test == "arith")	
		elapsed_time = arith_test(batchSize, totalSize, numThreads);
	else
		return 0;

	double BW = totalSize * numThreads / elapsed_time;
	cout << "Elapsed time: " << elapsed_time << " s" << endl;
	cout << "Bandwidth: " << BW << " GB/s" << endl;
	
	return 0;
}


