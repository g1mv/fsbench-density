/*
 (C) 2011, Dell Inc. Written by Przemyslaw Skibinski (inikep@gmail.com)

 LICENSE

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details at
 Visit <http://www.gnu.org/copyleft/gpl.html>.

 Written by Przemyslaw Skibinski, heavily modified by m^2.
 Parsing command line is the only part that bears any resemblence to the original,
 therefore if it's copyrightable, the license above applies to it.
 As for the rest, it's public domain.
 If your country doesn't recognize author's right to relieve themselves of copyright,
 you can use it under the terms of WTFPL version 2.0 or later.
 */

///////////////////////////
// DEFINITIONS
///////////////////////////
#define PROGNAME "fsbench"
#define PROGVERSION "0.14"

///////////////////////////
// INCLUDES
///////////////////////////

#include "benchmark.hpp"
#include "tools.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <new>

using namespace std;

///////////////////////////
// HELPERS
///////////////////////////

static vector<string> pretty_list_of_codecs()
{
    vector<string> ret;
    for (list<Codec*>::const_iterator it = CODECS.begin(); it != CODECS.end(); ++it)
    {
        const string & name = (*it)->name;
        const string & version = (*it)->version;
        if (name.find('/') == string::npos)
            ret.push_back(name + ' ' + version);
    }
    sort(ret.begin(), ret.end());
    return ret;
}

static void usage()
{
    cerr << PROGNAME " " PROGVERSION "\n\n";
    cerr << "usage1: " PROGNAME " help codec_name\n";
    cerr << "usage2: " PROGNAME " [list of codecs][options] input\n\n";
    cerr << "list of codecs: a space-separated list of codecs.\n";
    cerr << "                Each codec is either codec name or a comma separated\n";
    cerr << "                pair: name,params. See examples below.\n";
    cerr << "                You can also use the following pseudocodecs:\n";
    cerr << "                  all:     all supported codecs\n";
    cerr << "                  fast:    the list of codecs I consider fast\n";
    cerr << "                  default: the list of codecs used by default\n";
    cerr << "options:\n";
    cerr << "  -bX: filesystem block size(default = " << DEFAULT_BSIZE << ")\n";
    cerr << "  -c: write output as csv\n";
    cerr << "  -iX: number of iterations (default = " << DEFAULT_ITERATIONS << ")\n";
    cerr << "  -jX: job size (default = " << DEFAULT_OVERHEAD_ITERATIONS << ")\n";
    cerr << "  -oX: number of overhead-reduction iterations (default = " << DEFAULT_OVERHEAD_ITERATIONS << ")\n";
    cerr << "  -mX: minimal savings, i.e. disk sector size(default = " << DEFAULT_SSIZE << ")\n";
    cerr << "  -sX: minimum time taken by a single iteration (default = " << DEFAULT_ITER_TIME << ")\n";
    cerr << "  -tX: number of threads to use(default = " << DEFAULT_THREADS_NO << ")\n";
    cerr << "  -v: verify that decoding went fine\n";
    cerr << "  -wX: warmup iterations(default = " << DEFAULT_WARMUP_ITERS << ")\n";
    cerr << endl;
    cerr << "Available codecs:\n";
    vector<string> codecs_list = pretty_list_of_codecs();
    for (vector<string>::iterator it = codecs_list.begin(); it != codecs_list.end(); ++it)
        cerr << "  " << *it << endl;
    cerr << endl;
    cerr << "Examples:\n";
    cerr << "  " PROGNAME " help lzo\n";
    cerr << "  " PROGNAME " file.tar\n";
    cerr << "  " PROGNAME " -t2 file.tar\n";
    cerr << "  " PROGNAME " LZ4 zlib,1 -t2 file.tar\n";
    cerr << "  " PROGNAME " fastlz lzf,ultra -t2 -b131072 -m4096 -i10 file.tar\n";
}


///////////////////////////
// MAIN
///////////////////////////
int main(int argc, char ** argv)
{
    // increase priority to the max to reduce variance
    setHighestPriority();

    unsigned long iterations = DEFAULT_ITERATIONS;
    unsigned long overhead_iterations = DEFAULT_OVERHEAD_ITERATIONS;
    size_t bsize = DEFAULT_BSIZE;
    size_t ssize = DEFAULT_SSIZE;
    bool verify = false;
    int threads = DEFAULT_THREADS_NO;
    int warmup_iters = DEFAULT_WARMUP_ITERS;
    bool csv = false; // should the output be written as csv or human readably?
    unsigned iter_time = DEFAULT_ITER_TIME;
    size_t job_size = 262144; // when compressing small blocks, each work item will contain multiple of them, so the size is no smaller than this

    list<CodecWithParams> codecs;

    //debug code
    /*char*a[] = {"fsbench", "ar", "-w0", "-s1", "-i1", "/usr/home/m/bench/nbbs.tar"};
    argv=a;
    argc=6;*/

    if (argc == 3)
    {
        if (!case_insensitive_compare(argv[1], "help"))
        {
            Codec * c = find_codec(argv[2]);
            if (c)
            {
                cout << c->help();
                return 0;
            }

            cerr << "Unknown codec: '" << argv[2] << "'";
            return 1;
        }
    }

    while (argc > 2)
    {
        if (argv[1][0] == '-')
        {
            switch (argv[1][1])
            {
            case 'b':
                bsize = atoi(argv[1] + 2);
                if (bsize <= 0)
                {
                    cerr << "block size must be > 0.\n";
                    return 1;
                }
            break;
            case 'c':
                csv = true;
            break;
            case 'i':
                iterations = atoi(argv[1] + 2);
                if (iterations <= 0)
                {
                    cerr << "number of iterations must be > 0.\n";
                    return 1;
                }
            break;
            case 'j':
                job_size = atoi(argv[1] + 2);
                if (job_size <= 0)
                {
                    cerr << "job size must be > 0.\n";
                    return 1;
                }
            break;
            case 'm':
                ssize = atoi(argv[1] + 2);
                if (ssize <= 0)
                {
                    cerr << "minimum savings must be > 0.\n";
                    return 1;
                }
            break;
            case 'o':
                overhead_iterations = atoi(argv[1] + 2);
                if (overhead_iterations == 0)
                {
                    cerr << "number of overhead-reduction iterations must be > 0.\n";
                    return 1;
                }
            break;
            case 's':
                iter_time = atoi(argv[1] + 2);
            break;
            case 't':
                threads = atoi(argv[1] + 2);
                if (threads <= 0)
                {
                    cerr << "number of threads must be > 0.\n";
                    return 1;
                }
            break;
            case 'v':
                verify = true;
            break;
            case 'w':
                warmup_iters = atoi(argv[1] + 2);
                if (warmup_iters < 0)
                {
                    cerr << "warmup_iters of threads must be >= 0.\n";
                    return 1;
                }
            break;
            default:
                cerr << "unknown option: '" << argv[1] << "'\n";
                return 1;
            }
        }
        else
        {
            // it may be "codec" or "codec,params" or "pseudocodec".
            // pseudocodec?
            if (case_insensitive_compare(argv[1], "all") == 0)
            {
                codecs.insert(codecs.end(), ALL_COMPRESSORS.begin(), ALL_COMPRESSORS.end());
                codecs.insert(codecs.end(), ALL_CHECKSUMS.begin(), ALL_CHECKSUMS.end());
                codecs.insert(codecs.end(), ALL_CIPHERS.begin(), ALL_CIPHERS.end());
                codecs.insert(codecs.end(), ALL_OTHERS.begin(), ALL_OTHERS.end());
            }
            else if (case_insensitive_compare(argv[1], "default") == 0)
            {
                codecs.insert(codecs.end(), DEFAULT_CODECS.begin(), DEFAULT_CODECS.end());
            }
            else if (case_insensitive_compare(argv[1], "fast") == 0)
            {
                codecs.insert(codecs.end(), FAST_COMPRESSORS.begin(), FAST_COMPRESSORS.end());
            }
            else if (case_insensitive_compare(argv[1], "compressors") == 0)
            {
                codecs.insert(codecs.end(), ALL_COMPRESSORS.begin(), ALL_COMPRESSORS.end());
            }
            else if (case_insensitive_compare(argv[1], "checksums") == 0)
            {
                codecs.insert(codecs.end(), ALL_CHECKSUMS.begin(), ALL_CHECKSUMS.end());
            }
            else if (case_insensitive_compare(argv[1], "ciphers") == 0)
            {
                codecs.insert(codecs.end(), ALL_CIPHERS.begin(), ALL_CIPHERS.end());
            }
            else if (case_insensitive_compare(argv[1], "others") == 0)
            {
                codecs.insert(codecs.end(), ALL_OTHERS.begin(), ALL_OTHERS.end());
            }
            else
            {
                // codec?
                string codec = argv[1];
                string params = "";
                size_t position = codec.find(',');
                if (position != string::npos)
                {
                    params = codec.substr(position + 1);
                    codec = codec.substr(0, position);
                }
                Codec * found = find_codec(codec);
                if (found)
                {
                    codecs.push_back(CodecWithParams(*found, params));
                }
                else
                {
                    cerr << "Unknown codec: '" << codec << "'.";
                    return 1;
                }
            }
        }
        argv++;
        argc--;
    }

    if (argc < 2)
    {
        usage();
        return 1;
    }

    ifstream in(argv[1], ios::in | ios::binary);
    if (in.fail())
    {
        cerr << "Failed to open the input file.\n";
        return 1;
    }

    if (codecs.empty())
        codecs = DEFAULT_CODECS;

    if (codecs.empty())
    {
        cerr << "No valid codecs specified.\n";
        return 1;
    }

    try
    {
        Tester tester(codecs, &in, bsize, threads);
        tester.test(iterations,
                    overhead_iterations,
                    iter_time,
                    ssize,
                    verify,
                    warmup_iters,
                    csv,
                    job_size);

    }
    catch (const Codec::InvalidParams & e)
    {
        cerr << "Invalid params!\n";
        cerr << e.what() << '\n';
        return 1;
    }
    catch (const std::bad_alloc)
    {
        cerr << "Out of memory\n";
        return 1;
    }
    catch (const std::exception & e)
    {
        cerr << e.what();
        return 1;
    }
    catch (...)
    {
        cerr << "Exception caught";
        return 1;
    }
    if (csv)
    {
        cout << "Iterations," << iterations << "\n";
        cout << "Overhead iterations," << overhead_iterations << "\n";
    }
    else
    {
        cout << "done... (" << iterations << "*X*" << overhead_iterations << ") iteration(s)).\n";
    }
    return 0;
}
