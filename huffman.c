/* Title: Huffman coding for text files
 * URL: https://github.com/Ali-Zol/Huffman-coding-text-in-C
 * Date: 01/02/2024
 * Author: Ali-Zol
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_ALPHABETS 128

typedef struct Node
{
    char ch;
    unsigned freq;
    struct Node *left_node, *right_node;
} Node;

typedef struct
{
    int size;
    Node** arr;
} minHeap;

typedef struct
{
    char ch;
    char code[64];
    int codeLen;
} CharCode;


Node* newNode(char, unsigned);
void readFile(FILE*, Node*, unsigned*);
minHeap* createMinHeap_unsorted(Node*, int);
void minHeapify(minHeap*, int);
void swap_nodePtr(Node**, Node**);
Node* extractMin(minHeap*);
void insertNode(minHeap*, Node*);
void findCharCode(Node*, CharCode**, int);
void writeCharCode(FILE*, CharCode**, int);
void compressFile(FILE*, FILE*, CharCode**);
void extractCodesFromCompressedFile(FILE*, CharCode**, int);
Node* rebuildHuffmanTree(CharCode**, int);
void decompressFile(FILE*, FILE*, Node*);


int main(int argc, char* argv[])
{
    // Help menu.
    if(!strcmp(argv[1], "--help"))
    {
        puts("Usage: ./huffman [OPTION] FILE1 FILE2");
        puts("Compress or decompress \"FILE1\" to \"FILE2\".");
        putchar('\n');

        puts("Mandatory arguments to long options are mandatory for short options too.");
        puts("  -c, -C, --compress\t\tcompress FILE1 to FILE2");
        puts("  -d, -D, --decompress\t\tdecompress FILE1 to FILE2");
        puts("          --help\t\tdisplay this help and exit");
        putchar('\n');
        puts("if FILE2 does not exist, huffman makes it.");

        return 0;
    }
    
    // Error: less or more arguments.
    else if(argc != 4)
    {
        printf("%s: \033[1;31m%s: \033[1;0m", "huffman", "Error");
        puts((argc < 4) ? ("Too less arguments supplied") : ("Too many arguments supplied"));
        puts("Try 'huffman --help' for more information.");
    
        return -1;
    }

    // Compressing:
    else if(!strcmp(argv[1], "--compress") || !strcmp(argv[1], "-c") || !strcmp(argv[1], "-C"))     
    {
        // Open input file to compress it.
        FILE* filePtr1 = fopen(argv[2], "r");

        if(filePtr1 == NULL)
        {
            perror("\033[1;31mOpen failed for input file\033[1;0m");

            return -1;
        }
        printf("\033[1;32m%s \033[1;33m%s\033[1;0m\n", "Compressing!", "Please wait...");

        unsigned size = 0;
        Node node[NUM_ALPHABETS];

        for(int i = 0; i < NUM_ALPHABETS; i++)
        {
            node[i].freq = 0;
            node[i].left_node = node[i].right_node = NULL;
        }

        // Read input file and calculate the characters and their frequency in the form of node
        //and the number of distinct character in the form of size.
        readFile(filePtr1, node, &size);

        // It puts the data in a min heap but it's still unsorted.
        minHeap* minHeapPtr = createMinHeap_unsorted(node, size);

        // Sorting min heap(min heapify). It sorts by letter frequency.
        for(int i = ((size - 1) - 1) / 2; i >= 0; i--)
            minHeapify(minHeapPtr, i);

        // Building Huffman tree.
        while(minHeapPtr->size != 1)
        {
            // Extract letter with minimum frequency from min heap.
            Node* left = extractMin(minHeapPtr);        
            Node* right = extractMin(minHeapPtr);
                
            // It make a new node that located on the top of left and right node.
            // Node's character is '$', because it is not in the input file and
            // the node's frequency is the sum of left's and right's frequency.
            Node* top = newNode('$', left->freq + right->freq);
            top->left_node = left;
            top->right_node = right;

            // Insert the new node to the min heap and sort it again.
            insertNode(minHeapPtr, top);
        }

        // To save any character with its Huffman code.
        CharCode* character_code[NUM_ALPHABETS];

        int index = 0;
        while(index < size)
        {   
            findCharCode(minHeapPtr->arr[0], &character_code[index], 0);
            if(character_code[index]->ch)
                index++;
        }

        if(!strstr(argv[3], ".hfm"))
            strcat(argv[3], ".hfm");

        // Open output file to write the table of codes and compressed file.
        FILE* filePtr2 = fopen(argv[3], "w");                   

        // Print 0 in first line of output file to save the number of compressed bits
        //later to decompress it correctly. Print size to save the number of
        //distinct characters of input file.
        fprintf(filePtr2, "%d\n%d\n", 0, size);

        // Write the characters and their codes in the output file.
        writeCharCode(filePtr2, character_code, size);

        compressFile(filePtr1, filePtr2, character_code);

        printf("\033[1;32m%s\033[1;0m\n", "Compressing completed!");

        fclose(filePtr1);
        fclose(filePtr2);
        return 0;
    }

    // Decompressing:
    else if(!strcmp(argv[1], "--decompress") || !strcmp(argv[1], "-d") || !strcmp(argv[1], "-D"))
    {
        // Open input file to decompress it.
        FILE* filePtr1 = fopen(argv[2], "r");

        if(filePtr1 == NULL)
        {
            perror("huffman: \033[1;31mOpen failed for input file\033[1;0m");

            return -1;
        }
        
        if(!strstr(argv[2], ".hfm"))
        {
            printf("huffman: \033[1;31m%s: \033[1;0m%s\n", "The file name is incorrect", "File name must be <file_name>.hfm");

            return -1;
        }
        printf("\033[1;32m%s \033[1;33m%s\033[1;0m\n", "Decompressing!", "Please wait...");

        CharCode* character_code[NUM_ALPHABETS];
        int size;

        // Scan the number of distinct characters from input file.
        fseek(filePtr1, 2, SEEK_SET);
        fscanf(filePtr1, "%d", &size);
        fseek(filePtr1, 1, SEEK_CUR);
        
        // Extract characters and their codes from compressed file.
        extractCodesFromCompressedFile(filePtr1, character_code, size);  

        Node* treeRoot = rebuildHuffmanTree(character_code, size);    

        // Open output file to decompressed input file.
        FILE* filePtr2 = fopen(argv[3], "w");

        decompressFile(filePtr1, filePtr2, treeRoot);

        printf("\033[1;32m%s\033[1;0m\n", "Decompressing completed!");

        fclose(filePtr1);
        fclose(filePtr2);
        return 0;
    }

    // Error: Invalid option.
    else        
    {
        printf("huffman:\033[1;31m invalid option -- \"%s\"\033[1;0m\n", argv[1]);
        puts("Try 'huffman --help' for more information.");

        return -1;
    }

    return 0;
}

void readFile(FILE* filePtr, Node node[], unsigned* size)
{
    char temp;
    for(int i = 0; (temp = fgetc(filePtr)) != EOF; i++)
    {
        int idx = i;
        int flag = 1;

        for(int j = 0; j < i; j++)
        {
            if(node[j].ch == temp)
            {
                idx = j;
                flag = 0;
                i--;
                break;
            }
        }

        node[idx].freq++;
        if(flag)
        {
            node[idx].ch = temp;
            (*size)++;
        }
    }
}

Node* newNode(char c, unsigned freq)
{
    Node* temp = (Node*)malloc(sizeof(Node));
   
    temp->left_node = temp->right_node = NULL;
    temp->ch = c;
    temp->freq = freq;
    
    return temp;
}

minHeap* createMinHeap_unsorted(Node node[], int size)
{
    minHeap* minHeapPtr = (minHeap*)malloc(sizeof(minHeap));

    minHeapPtr->size = size;

    minHeapPtr->arr = (Node**)malloc(size * sizeof(Node*));

    for(int i = 0; i < size; i++)
        minHeapPtr->arr[i] = &node[i];

    return minHeapPtr;
}

// Create a min heap from an unsorted array.
void minHeapify(minHeap* minHeapPtr, int i)
{
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if((left < minHeapPtr->size) && (minHeapPtr->arr[left]->freq < minHeapPtr->arr[smallest]->freq))
        smallest = left;

    if((right < minHeapPtr->size) && (minHeapPtr->arr[right]->freq < minHeapPtr->arr[smallest]->freq))
        smallest = right;

    if(smallest != i)
    {
        swap_nodePtr(&minHeapPtr->arr[smallest], &minHeapPtr->arr[i]);
        minHeapify(minHeapPtr, smallest);
    }
}

void swap_nodePtr(Node** a, Node** b)
{
    Node * temp = *a;
    *a = *b;
    *b = temp;
}


Node* extractMin(minHeap* minHeapPtr)
{
    Node* minNode = minHeapPtr->arr[0];
    minHeapPtr->arr[0] = minHeapPtr->arr[--minHeapPtr->size];

    minHeapify(minHeapPtr, 0);

    return minNode;
}

void insertNode(minHeap* minHeapPtr, Node* insertedNode)
{
    minHeapPtr->arr[minHeapPtr->size++] = insertedNode;

    for (int i = minHeapPtr->size - 1; i && minHeapPtr->arr[i]->freq < minHeapPtr->arr[(i - 1) / 2]->freq; i = (i - 1) / 2)
        swap_nodePtr(&minHeapPtr->arr[i], &minHeapPtr->arr[(i - 1) / 2]);
}

void findCharCode(Node* root, CharCode** arr, int top)
{
    if(!top)
    {
        (*arr) = (CharCode*)malloc(sizeof(CharCode));
        (*arr)->ch = '\0';
    }

    if(root->left_node != NULL && root->left_node->ch > 9)
    {
        (*arr)->code[top] = 0;
        findCharCode(root->left_node, arr, top + 1);
        return;
    }

    if(root->right_node != NULL && root->right_node->ch > 9)
    {
        (*arr)->code[top] = 1;
        findCharCode(root->right_node, arr, top + 1);
        return;
    }

    if(root->ch != '$')
    {
        (*arr)->ch = root->ch;
        (*arr)->codeLen = top;
    }

    root->ch = '\0';
    root->freq = 0;
}

void writeCharCode(FILE* myFile, CharCode* arr[], int size)
{
    for(int i = 0; i < size; i++)
    {
        fputc(arr[i]->ch, myFile);

        for(int j = 0; j < arr[i]->codeLen; j++)
            fprintf(myFile, "%d", arr[i]->code[j]);

        fputc('\n', myFile);
    }
}

void compressFile(FILE* input, FILE* output, CharCode* arr[])
{
    char byte = 0;
    int compressedBits = 0;
    char temp;

    fseek(input, 0, SEEK_SET);
    while((temp = fgetc(input)) != EOF)
    {
        int idx;
        for(idx = 0; temp != arr[idx]->ch; idx++);
        for(int i = 0; i < arr[idx]->codeLen; i++)
        {
            byte<<= 1;
            byte|= arr[idx]->code[i];
            compressedBits++;

            if(compressedBits == 7)
            {
                fputc(byte, output);
                compressedBits = 0;
                byte = 0;
            }
        }   
    }

    if(compressedBits != 0)
    {
        byte<<= 7 - compressedBits;
        fputc(byte, output);
        fseek(output, 0, SEEK_SET);
        fprintf(output, "%d", compressedBits + 1);
    }
}

void extractCodesFromCompressedFile(FILE* input, CharCode* arr[], int size)
{
    for(int i = 0; i < size; i++)
    {
        arr[i] = (CharCode*)malloc(sizeof(CharCode));
        arr[i]->ch = fgetc(input);

        int j;
        char temp;
        for(j = 0; (temp = fgetc(input)) != '\n'; j++)
            arr[i]->code[j] = temp;

        arr[i]->codeLen = j;
    }
}

Node* rebuildHuffmanTree(CharCode* arr[], int size)
{
    Node* root = (Node*)malloc(sizeof(Node));
    root->ch = '$';

    for(int i = 0; i < size; i++)
    {
        Node* curr = root;

        for(int j = 0; j < arr[i]->codeLen; j++)
        {
            curr->ch = '$';

            if(arr[i]->code[j] == '0')
            {
                if(!curr->left_node)
                    curr->left_node = (Node*)malloc(sizeof(Node));

                curr = curr->left_node;
            }
            else
            {
                if(!curr->right_node)
                    curr->right_node = (Node*)malloc(sizeof(Node));
            
                curr = curr->right_node;
            }
        }

        curr->ch = arr[i]->ch;
    }

    return root;
}

void decompressFile(FILE* input, FILE* output, Node* root)
{
    int temp;
    Node* curr = root;
    
    while((temp = fgetc(input)) != EOF)
    {
        short byte[7] = {0};

        for (int i = 6; temp; i--)
        {
            byte[i] = temp % 2;
            temp/= 2;
        }

        char checkEnd;

        if((checkEnd = fgetc(input)) == EOF)
        {
            fseek(input, 0, SEEK_SET);

            int compressedBits;
            fscanf(input, "%d", &compressedBits);

            for(int i = 0; i < compressedBits; i++)
            {
                if(byte[i])
                    curr = curr->right_node;

                else
                    curr = curr->left_node;

                if(curr->ch != '$')
                {
                    fputc(curr->ch, output);
                    curr = root;
                }
            }
            break;
        }

        fseek(input, -1, SEEK_CUR);

        for(int i = 0; i < 7; i++)
        {
            if(byte[i])
                curr = curr->right_node;

            else
                curr = curr->left_node;

            if(curr->ch != '$')
            {
                fputc(curr->ch, output);
                curr = root;
            }
        }
    }
}