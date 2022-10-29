/**********************************************************
 * File: HuffmanEncoding.cpp
 *
 * Implementation of the functions from HuffmanEncoding.h.
 * Most (if not all) of the code that you write for this
 * assignment will go into this file.
 */

#include "HuffmanEncoding.h"
#include "foreach.h"
#include <string>
#include "strlib.h"
#include "map.h"

/* Function: getFrequencyTable
 * Usage: Map<ext_char, int> freq = getFrequencyTable(file);
 * --------------------------------------------------------
 * Given an input stream containing text, calculates the
 * frequencies of each character within that text and stores
 * the result as a Map from ext_chars to the number of times
 * that the character appears.
 *
 * This function will also set the frequency of the PSEUDO_EOF
 * character to be 1, which ensures that any future encoding
 * tree built from these frequencies will have an encoding for
 * the PSEUDO_EOF character.
 */
Map<ext_char, int> getFrequencyTable(istream& file) 
{
	Map<ext_char, int> freqTable;
	ext_char currChar;

	while (true)
	{
		currChar = file.get();
		if (currChar == EOF) break;

		if (freqTable.containsKey(currChar))
		{
			//increase stats
			freqTable[currChar] += 1;
		}
		else
		{
			//put new character
			freqTable[currChar] = 1;
		}
	}

	freqTable[PSEUDO_EOF] = 1;

	return freqTable;
}

/* Function: buildEncodingTree
 * Usage: Node* tree = buildEncodingTree(frequency);
 * --------------------------------------------------------
 * Given a map from extended characters to frequencies,
 * constructs a Huffman encoding tree from those frequencies
 * and returns a pointer to the root.
 *
 * This function can assume that there is always at least one
 * entry in the map, since the PSEUDO_EOF character will always
 * be present.
 */
Node* buildEncodingTree(Map<ext_char, int>& frequencies) 
{
	PriorityQueue<Node*> pQueue;
	enqueueNodes(frequencies, pQueue);		//add nodes in queue
	Node* result = mergeNodes(pQueue);		//merge nodes and build encoding tree
	
	return result;
}

/* Function: freeTree
 * Usage: freeTree(encodingTree);
 * --------------------------------------------------------
 * Deallocates all memory allocated for a given encoding
 * tree.
 */
void freeTree(Node* root) 
{
	if (root->character != NOT_A_CHAR) //base case
	{
		delete root;
		return;
	}

	freeTree(root->zero);	//delete left tree
	freeTree(root->one);	//delete right tree

	delete root;	//delete root of tree
}

/* Function: encodeFile
 * Usage: encodeFile(source, encodingTree, output);
 * --------------------------------------------------------
 * Encodes the given file using the encoding specified by the
 * given encoding tree, then writes the result one bit at a
 * time to the specified output file.
 *
 * This function can assume the following:
 *
 *   - The encoding tree was constructed from the given file,
 *     so every character appears somewhere in the encoding
 *     tree.
 *
 *   - The output file already has the encoding table written
 *     to it, and the file cursor is at the end of the file.
 *     This means that you should just start writing the bits
 *     without seeking the file anywhere.
 */ 
void encodeFile(istream& infile, Node* encodingTree, obstream& outfile) 
{
	Map<ext_char, string> encodingMap;
	ext_char currChar;
	string code;

	while (true)
	{
		currChar = infile.get(); //get character

		if (currChar == EOF) break;
		
		if (encodingMap.containsKey(currChar))
		{
			/*
				if program already searched this character then this
				character must be in encodingMap
			*/
			code = encodingMap[currChar];
		}
		else
		{
			//if program try to search character first time
			code = searchCharacterInTree(encodingTree, currChar);
			encodingMap[currChar] = code; //put new character in map
		}

		writeBits(outfile, code); //write bits in file one by one
	}

	code = searchCharacterInTree(encodingTree, PSEUDO_EOF);
	writeBits(outfile, code);
}

/* Function: decodeFile
 * Usage: decodeFile(encodedFile, encodingTree, resultFile);
 * --------------------------------------------------------
 * Decodes a file that has previously been encoded using the
 * encodeFile function.  You can assume the following:
 *
 *   - The encoding table has already been read from the input
 *     file, and the encoding tree parameter was constructed from
 *     this encoding table.
 *
 *   - The output file is open and ready for writing.
 */
void decodeFile(ibstream& infile, Node* encodingTree, ostream& file) 
{
	int bit;
	string code = "";
	ext_char character;

	while (true)
	{
		bit = infile.readBit(); //read bit

		code += integerToString(bit); //save this one bit in code
		character = searchCodeInTree(encodingTree, code); //try to search bit with code
		if (character == PSEUDO_EOF) break;

		if (character != NOT_A_CHAR)
		{
			file.put(character);
			code = "";
		}
	}
}

/* Function: writeFileHeader
 * Usage: writeFileHeader(output, frequencies);
 * --------------------------------------------------------
 * Writes a table to the front of the specified output file
 * that contains information about the frequencies of all of
 * the letters in the input text.  This information can then
 * be used to decompress input files once they've been
 * compressed.
 *
 * This function is provided for you.  You are free to modify
 * it if you see fit, but if you do you must also update the
 * readFileHeader function defined below this one so that it
 * can properly read the data back.
 */
void writeFileHeader(obstream& outfile, Map<ext_char, int>& frequencies) {
	/* The format we will use is the following:
	 *
	 * First number: Total number of characters whose frequency is being
	 *               encoded.
	 * An appropriate number of pairs of the form [char][frequency][space],
	 * encoding the number of occurrences.
	 *
	 * No information about PSEUDO_EOF is written, since the frequency is
	 * always 1.
	 */
	 
	/* Verify that we have PSEUDO_EOF somewhere in this mapping. */
	if (!frequencies.containsKey(PSEUDO_EOF)) {
		error("No PSEUDO_EOF defined.");
	}
	
	/* Write how many encodings we're going to have.  Note the space after
	 * this number to ensure that we can read it back correctly.
	 */
	outfile << frequencies.size() - 1 << ' ';
	
	/* Now, write the letter/frequency pairs. */
	foreach (ext_char ch in frequencies) {
		/* Skip PSEUDO_EOF if we see it. */
		if (ch == PSEUDO_EOF) continue;
		
		/* Write out the letter and its frequency. */
		outfile << char(ch) << frequencies[ch] << ' ';
	}
}

/* Function: readFileHeader
 * Usage: Map<ext_char, int> freq = writeFileHeader(input);
 * --------------------------------------------------------
 * Reads a table to the front of the specified input file
 * that contains information about the frequencies of all of
 * the letters in the input text.  This information can then
 * be used to reconstruct the encoding tree for that file.
 *
 * This function is provided for you.  You are free to modify
 * it if you see fit, but if you do you must also update the
 * writeFileHeader function defined before this one so that it
 * can properly write the data.
 */
Map<ext_char, int> readFileHeader(ibstream& infile) {
	/* This function inverts the mapping we wrote out in the
	 * writeFileHeader function before.  If you make any
	 * changes to that function, be sure to change this one
	 * too!
	 */
	Map<ext_char, int> result;
	
	/* Read how many values we're going to read in. */
	int numValues;
	infile >> numValues;
	
	/* Skip trailing whitespace. */
	infile.get();
	
	/* Read those values in. */
	for (int i = 0; i < numValues; i++) {
		/* Get the character we're going to read. */
		ext_char ch = infile.get();
		
		/* Get the frequency. */
		int frequency;
		infile >> frequency;
		
		/* Skip the space character. */
		infile.get();
		
		/* Add this to the encoding table. */
		result[ch] = frequency;
	}
	
	/* Add in 1 for PSEUDO_EOF. */
	result[PSEUDO_EOF] = 1;
	return result;
}

/* Function: compress
 * Usage: compress(infile, outfile);
 * --------------------------------------------------------
 * Main entry point for the Huffman compressor.  Compresses
 * the file whose contents are specified by the input
 * ibstream, then writes the result to outfile.  Your final
 * task in this assignment will be to combine all of the
 * previous functions together to implement this function,
 * which should not require much logic of its own and should
 * primarily be glue code.
 */
void compress(ibstream& infile, obstream& outfile) 
{
	Map<ext_char, int> frequencyTable = getFrequencyTable(infile);
	writeFileHeader(outfile, frequencyTable);
	Node* rootEncodingTree = buildEncodingTree(frequencyTable);
	infile.rewind();
	encodeFile(infile, rootEncodingTree, outfile);
	freeTree(rootEncodingTree);
}

/* Function: decompress
 * Usage: decompress(infile, outfile);
 * --------------------------------------------------------
 * Main entry point for the Huffman decompressor.
 * Decompresses the file whose contents are specified by the
 * input ibstream, then writes the decompressed version of
 * the file to the stream specified by outfile.  Your final
 * task in this assignment will be to combine all of the
 * previous functions together to implement this function,
 * which should not require much logic of its own and should
 * primarily be glue code.
 */
void decompress(ibstream& infile, ostream& outfile) 
{
	Map<ext_char, int> frequencyTable = readFileHeader(infile); 
	Node* rootEncodingTree = buildEncodingTree(frequencyTable);
	decodeFile(infile, rootEncodingTree, outfile);
	freeTree(rootEncodingTree);
}


		/*	Private functions ipleentation	*/

/*
	This function creates Nodes with correct parameters and 
	adds them in priority queue
*/
void enqueueNodes(Map<ext_char, int>& frequencies, PriorityQueue<Node*>& pQueue)
{
	Node* node;

	foreach(ext_char currChar in frequencies)
	{
		node = new Node;
		node->zero = NULL;
		node->one = NULL;
		node->character = currChar;
		node->weight = frequencies[currChar];
		pQueue.enqueue(node, node->weight);
	}
}

/*	
	This function merges Nodes, builds encoding tree and 
	returns root of tree
*/
Node* mergeNodes(PriorityQueue<Node*>& pQueue)
{
	Node* node;

	while (true)
	{
		if (pQueue.size() == 1) break;

		node = new Node;
		node->character = NOT_A_CHAR;
		node->zero = pQueue.dequeue();
		node->one = pQueue.dequeue();
		node->weight = node->zero->weight + node->one->weight;
		pQueue.enqueue(node, node->weight);
	}

	Node* result = pQueue.dequeue();
	
	return result;
}

/*
	this functions searches character in tree and 
	returns relevant code of character
*/
string searchCharacterInTree(Node* root, ext_char character)
{	
	if (root->character != NOT_A_CHAR) return "";

	string result = "";

	result += "0";
	result += searchCharacterInTree(root->zero, character);
	if (isCorrectCode(root, result, character)) return result;
	result = "";
	
	result += "1";
	result += searchCharacterInTree(root->one, character);
	if (isCorrectCode(root, result, character)) return result;

	return "";
}    

/*
	Tis function checks code, if code is correct sequence of bits from 
	root then functions returns true if not returns false
*/
bool isCorrectCode(Node* fromRoot, string code, ext_char character)
{
	Node* currNode = fromRoot;

	for (int i = 0; i < code.size(); i++)
	{
		if (code[i] == '0')
		{
			currNode = currNode->zero;
		}
		else
		{
			currNode = currNode->one;
		}
	}

	if (currNode->character == character) return true;
	
	return false;
}

/*
	This functions writes some bits in file
*/
void writeBits(obstream& outfile, string code)
{
	int bit;

	for (int i = 0; i < code.size(); i++)
	{
		if (code[i] == '0')
		{
			bit = 0;
		}
		else
		{
			bit = 1;
		}

		outfile.writeBit(bit);
	}
}

/*
	this function searches code in tree and returns relevant character
*/
ext_char searchCodeInTree(Node* root, string code)
{
	Node* currNode = root;

	for (int i = 0; i < code.size(); i++)
	{
		if (code[i] == '0')
		{
			currNode = currNode->zero;
		}
		else
		{
			currNode = currNode->one;
		}
	}

	return currNode->character;
}