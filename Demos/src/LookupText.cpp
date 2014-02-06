/*
  LookupText - a demo program for libosmscout
  Copyright (C) 2013  Preet Desai

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include <osmscout/TextSearch.h>

void badInput()
{
    std::cout << "ERROR: Bad arguments" << std::endl;
    std::cout << "* Pass in the directory containing the text data files,\n"
                 "  ie. text(poi,loc,region,other).dat" << std::endl;
    std::cout << "ex:" << std::endl;
    std::cout << "./LookupText /path/to/imported/map/data" << std::endl;
}

int main (int argc, char *argv[])
{
    if(argc != 2) {
        badInput();
        return -1;
    }

    // load text data files
    std::string path(argv[1]);
    osmscout::TextSearch textSearch;
    if(!textSearch.Load(path)) {
        std::cout << "ERROR: Failed to load text files!"
                     "(are you sure you passed the right path?)"
                  << std::endl;
        return -1;
    }

    std::cout << "* Searches are case-sensitive\n"
                 "* Displays up to 10 unique text results\n"
                 "* Displays up to 5 file offsets for each result\n"
                 "* Input at least 3 characters or 'q' to quit\n" << std::endl;


    while(true) {
        std::cout << std::endl;
        std::cout << "Enter a search term:"<<std::endl;
        std::string search_input;
        std::getline(std::cin,search_input);

        if(search_input.size() < 3) {
            if(search_input.size()==1) {
                if(search_input[0]=='q' || search_input[0]=='Q') {
                    std::cout << "INFO: Quitting" << std::endl;
                    break;
                }
            }
            std::cout << "Input at least 3 characters" << std::endl;
            continue;
        }

        // search using the text input as the query
        osmscout::TextSearch::ResultsMap results;
        textSearch.Search(search_input,true,true,true,true,results);

        if(results.empty()) {
            std::cout << "No results found." << std::endl;
            continue;
        }

        // print out the results
        size_t print_count=0;
        osmscout::TextSearch::ResultsMap::iterator it;
        for(it=results.begin(); it != results.end(); ++it) {
            std::cout << "\"" <<it->first << "\" -> ";
            std::vector<osmscout::ObjectFileRef> &refs=it->second;
            std::size_t max_print_offsets=5;
            std::size_t min_ref_count=std::min(refs.size(),max_print_offsets);

            for(size_t r=0; r < min_ref_count; r++) {
                if(refs[r].GetType() == osmscout::refNode) {
                    std::cout << "N:" << refs[r].GetFileOffset() << " ";
                }
                else if(refs[r].GetType() == osmscout::refWay) {
                    std::cout << "W:" << refs[r].GetFileOffset() << " ";
                }
                else if(refs[r].GetType() == osmscout::refArea) {
                    std::cout << "A:" << refs[r].GetFileOffset() << " ";
                }
            }
            if(refs.size() > 10) {
                std::cout << "... " << (refs.size()-10) << " more offsets";
            }
            std::cout << std::endl;
            print_count++;
            if(print_count == 10) {
                break;
            }
        }

        if(results.size() > 10) {
            std::cout << "... " << results.size()
                      << " total unique text results found" << std::endl;
        }
    }

    return 0;
}
