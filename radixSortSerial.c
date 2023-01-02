#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define BASE 2
#define FALSE (1 == 0)
#define TRUE (1 == 1)

int *createArray(int size, int initialValue)
{
    int *arr = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        arr[i] = initialValue;
    }
    return arr;
}

int *getRandomArray(int size)
{
    int *arr = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        arr[i] = rand() % 1000000;
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
    return number % (divisor);
}

int *countSort(int *arr, int size, int digitSpan, int offset)
{
    int iter = power(BASE, digitSpan);
    int *digitValues = malloc(sizeof(int) * size);
    int *hist = createArray(iter, 0);
    int *preSum = createArray(iter, 0);
    int *preSumGroup = createArray(iter, 0);

    for (int i = 0; i < size; i++)
    {
        digitValues[i] = getDigitValue(arr[i], digitSpan, offset);
        hist[digitValues[i]]++;
        // 1001, 1001, 1101, 1100, 1110
        // 2, 3
        // 1, 3, 1, 0
    }

    for (int i = 1; i < iter; i++)
    {
        preSum[i] = hist[i - 1] + preSum[i - 1];
        // 0, 1, 4, 5
    }

    int *relOffset = createArray(size, 0);

    for (int i = 0; i < size; i++)
    {
        relOffset[i] = preSumGroup[digitValues[i]];
        preSumGroup[digitValues[i]]++;
        //
    }

    int *temp = malloc(sizeof(int) * size);
    for (int i = 0; i < size; i++)
    {
        temp[preSum[digitValues[i]] + relOffset[i]] = arr[i];
    }

    free(digitValues);
    free(hist);
    free(preSum);
    free(preSumGroup);
    free(relOffset);
    return temp;
}

int *radixSort(int *arr, int size, int digitSpan)
{
    int totalDigitCount = findMaxDigitCount(arr, size);
    int totalScan = totalDigitCount / digitSpan;
    for (int currScan = 0; currScan < totalScan; currScan++)
    {
        arr = countSort(arr, size, digitSpan, currScan);
    }

    if (totalDigitCount % digitSpan != 0)
    {
        arr = countSort(arr, size, (totalDigitCount % digitSpan), (totalDigitCount / (totalDigitCount % digitSpan)) - 1);
    }

    return arr;
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
    int arrSize = readArraySize(argc, argv);
    printf("[ARRAY SIZE]: %d\n", arrSize);
    int *arr = getRandomArray(arrSize);
    printf("[SORTED]: %7s\n", checkSorted(arr, arrSize) ? "TRUE" : "FALSE");
    clock_t t;
    t = clock();
    arr = radixSort(arr, arrSize, 1);
    t = clock() - t;
    printf("[SORTED]: %7s\n", checkSorted(arr, arrSize) ? "TRUE" : "FALSE");
    printf("Elapsed time is %f\n", ((double)t) / CLOCKS_PER_SEC);
    free(arr);
}