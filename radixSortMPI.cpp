#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include <random>
#include <sstream>

#define BASE 2
#define FALSE (1 == 0)
#define TRUE (1 == 1)

int np;

int *createArray(int size, int initialValue)
{
    int *arr = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        arr[i] = initialValue;
    }
    return arr;
}

int *getRandomArray(int size)
{
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 100000000.0);
    int *arr = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        arr[i] = (int)dis(gen);
    }
    return arr;
}

// TODO PARALLELIZE HERE
int findMax(int *arr, int size)
{
    int max = 0;
    for (int i = 0; i < size; i++)
    {
        if (arr[i] > max)
        {
            max = arr[i];
        }
    }
    return max;
}

/*
 *   Returns maximum digit count inside the array according to the base defined above
 *   ie. 8 on base 2 is 1000 => result is 4
 */
int findMaxDigitCount(int *arr, int size)
{
    int max = findMax(arr, size);
    int digitCount = 0;
    while (max > 0)
    {
        max /= BASE;
        digitCount++;
    }

    return digitCount;
}

int findMaxDigitCountFromNetwork(int *arr, int size, int rank)
{
    int localMDC = findMaxDigitCount(arr, size);
    // printf("I'm rank: %d and my local MDC: %d\n", rank, localMDC);

    int *localMDCs;

    if (rank == 0)
    {
        localMDCs = (int *)malloc(sizeof(int) * np);
    }

    MPI_Gather(&localMDC, 1, MPI_INT, localMDCs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    int max = 0;
    if (rank == 0)
    {
        for (int i = 0; i < np; i++)
        {
            if (localMDCs[i] > max)
            {
                max = localMDCs[i];
            }
        }
    }
    MPI_Bcast(&max, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // printf("I'm rank: %d and max digit count is = %d\n", rank, max);
    return max;
}

int power(int num, int pow)
{
    int result = 1;
    for (int i = 0; i < pow; i++)
    {
        result *= num;
    }
    return result;
}

/*
 * for example 1001 is the binary number (BASE 2), digitSpan is 2 and offset is 0 than it will return 01 (the first two digits) => 1
 * continues: offset is 1 than it will return 10 (the second two digits) => 2
 */
int getDigitValue(int number, int digitSpan, int offset)
{
    int divisor = power(BASE, digitSpan);
    for (int i = 0; i < offset; i++)
    {
        number /= divisor;
    }
    int res = number % (divisor);
    return res;
}

int *countSort(int *arr, int size, int digitSpan, int offset, int *returnSize)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int iter = power(BASE, digitSpan);
    int *digitValues = (int *)malloc(sizeof(int) * size);
    int *hist = createArray(iter, 0);
    int *preSum = createArray(iter, 0);
    int *preSumGroup = createArray(iter, 0);

    for (int i = 0; i < size; i++)
    {
        digitValues[i] = getDigitValue(arr[i], digitSpan, offset);
        hist[digitValues[i]]++;
    }

    for (int i = 1; i < iter; i++)
    {
        preSum[i] = hist[i - 1] + preSum[i - 1];
    }

    int *relOffset = createArray(size, 0);

    for (int i = 0; i < size; i++)
    {
        relOffset[i] = preSumGroup[digitValues[i]];
        preSumGroup[digitValues[i]]++;
    }

    int *temp = (int *)malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        temp[preSum[digitValues[i]] + relOffset[i]] = arr[i];
    }

    int **distMap = (int **)malloc(sizeof(int *) * np);
    int *distIndex = (int *)malloc(sizeof(int) * np);

    for (int i = 0; i < np; i++)
    {
        distMap[i] = (int *)malloc(sizeof(int) * hist[i]);
        distIndex[i] = 0;
    }

    for (int i = 0; i < size; i++)
    {
        int dVal = getDigitValue(temp[i], digitSpan, offset);
        distMap[dVal][distIndex[dVal]++] = temp[i];
    }

    int npNp = np * np;
    int *expHist = (int *)malloc(sizeof(int) * npNp);
    for (int i = 0; i < npNp; i++)
    {
        expHist[i] = 0;
    }

    MPI_Allgather(hist, np, MPI_INT, expHist, np, MPI_INT, MPI_COMM_WORLD);

    int *histIncoming = (int *)malloc(sizeof(int) * np);
    for (int i = 0; i < np; i++)
    {
        histIncoming[i] = 0;
    }

    int *displacement = (int *)malloc(sizeof(int) * np);

    int numIncoming = 0;

    for (int i = 0; i < np; i++)
    {
        int idx = (np * i) + rank;
        histIncoming[i] = expHist[idx];
        numIncoming += histIncoming[i];
        displacement[i] = 0;
        if (i > 0)
        {
            displacement[i] = displacement[i - 1] + histIncoming[i - 1];
        }
    }

    int *nextArr = (int *)malloc(sizeof(int) * numIncoming);

    // std::ostringstream out;

    // for (int i = 0; i < np; i++)
    // {
    //     out << " " << histIncoming[i];
    // }

    // std::cout << "rank: " << rank << out.str() << std::endl;

    for (int i = 0; i < np; i++)
    {
        MPI_Gatherv(distMap[i], hist[i], MPI_INT, nextArr, histIncoming, displacement, MPI_INT, i, MPI_COMM_WORLD);
    }

    free(digitValues);
    free(preSum);
    free(preSumGroup);
    free(relOffset);
    free(temp);
    free(distIndex);
    free(histIncoming);
    free(displacement);
    free(expHist);

    for (int i = 0; i < np; i++)
    {
        free(distMap[i]);
    }

    free(distMap);

    (*returnSize) = numIncoming;

    return nextArr;
}

int checkSorted(int *arr, int size)
{
    for (int i = 1; i < size; i++)
    {
        if (arr[i] < arr[i - 1])
        {
            return FALSE;
        }
    }
    return TRUE;
}

int readArraySize(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "--size") == 0)
        {
            return atoi(argv[i + 1]);
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int arrSize, *arr;

    double t1, t2;

    if (rank == 0)
    {
        t1 = MPI_Wtime();
        arrSize = readArraySize(argc, argv);
        arr = getRandomArray(arrSize);
        printf("[ARRAY SIZE]: %d\n", arrSize);
        printf("[SORTED]: %s\n", checkSorted(arr, arrSize) ? "TRUE" : "FALSE");
    }

    int digitSpan = (int)log2(np);
    MPI_Bcast(&arrSize, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int subSize = arrSize / np;
    int *subArr = (int *)malloc(sizeof(int) * subSize);

    MPI_Scatter(arr, subSize, MPI_INT, subArr, subSize, MPI_INT, 0, MPI_COMM_WORLD);

    int maxDigitCount = findMaxDigitCountFromNetwork(subArr, subSize, rank);

    int noExcessScanCount = maxDigitCount / digitSpan;

    for (int i = 0; i < noExcessScanCount + 1; i++)
    {
        int *next = countSort(subArr, subSize, digitSpan, i, &subSize);
        free(subArr);
        subArr = next;
    }

    int *expectedIncoming, *displacements, *sortedArr;

    if (rank == 0)
    {
        expectedIncoming = (int *)malloc(sizeof(int) * np);
    }

    MPI_Gather(&subSize, 1, MPI_INT, expectedIncoming, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        displacements = (int *)malloc(sizeof(int) * np);
        displacements[0] = 0;
        sortedArr = (int *)malloc(sizeof(int) * arrSize);
        for (int i = 1; i < np; i++)
        {
            displacements[i] = displacements[i - 1] + expectedIncoming[i - 1];
        }
    }

    MPI_Gatherv(subArr, subSize, MPI_INT, sortedArr, expectedIncoming, displacements, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        t2 = MPI_Wtime();
        printf("[SORTED]: %s\n", checkSorted(sortedArr, arrSize) ? "TRUE" : "FALSE");
        free(sortedArr);
        printf("Elapsed time = %f\n", t2 - t1);
    }

    free(subArr);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}