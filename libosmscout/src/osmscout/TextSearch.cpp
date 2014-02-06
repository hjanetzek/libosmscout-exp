#include <iostream>
#include <osmscout/TextSearch.h>

namespace osmscout
{

TextSearch::TextSearch()
{
    // no code
}

bool TextSearch::Load(std::string const &path)
{
    std::string proper_path=path;
    if(path[path.length()-1]!='/') {
        proper_path.push_back('/');
    }

    std::vector<std::string> list_dat_files;
    list_dat_files.push_back(proper_path+"textpoi.dat");
    list_dat_files.push_back(proper_path+"textloc.dat");
    list_dat_files.push_back(proper_path+"textregion.dat");
    list_dat_files.push_back(proper_path+"textother.dat");

    m_list_tries.push_back(&m_trie_poi);
    m_list_tries.push_back(&m_trie_loc);
    m_list_tries.push_back(&m_trie_region);
    m_list_tries.push_back(&m_trie_other);

    for(size_t i=0; i < m_list_tries.size(); i++) {
        // open/load the data file
        try {
            m_list_tries[i]->load(list_dat_files[i].c_str());
        }
        catch(marisa::Exception const &ex) {
            std::cerr << "Error opening " << list_dat_files[i] << ":";
            std::cerr << ex.what() << std::endl;
            return false;
        }
    }

    return true;
}


bool TextSearch::Search(std::string const &query,
                        bool const searchPOIs,
                        bool const searchLocations,
                        bool const searchRegions,
                        bool const searchOther,
                        ResultsMap &results) const
{
    results.clear();

    if(query.empty()) {
        return true;
    }

    std::vector<bool> list_search_groups;
    list_search_groups.push_back(searchPOIs);
    list_search_groups.push_back(searchLocations);
    list_search_groups.push_back(searchRegions);
    list_search_groups.push_back(searchOther);

    for(size_t i=0; i < m_list_tries.size(); i++) {
        if(list_search_groups[i]) {
            marisa::Agent agent;

            try {
                agent.set_query(query.c_str());
                while(m_list_tries[i]->predictive_search(agent)) {
                    std::string result(agent.key().ptr(),agent.key().length());
                    std::string text;
                    ObjectFileRef ref;

                    splitSearchResult(result,text,ref);

                    ResultsMap::iterator it=results.find(text);
                    if(it==results.end()) {
                        // If the text has not been added to the
                        // search results yet, insert a new entry
                        std::pair<std::string,std::vector<ObjectFileRef> > entry;
                        entry.first = text;
                        entry.second.push_back(ref);
                        results.insert(entry);
                    }
                    else {
                        // Else add the offset to the existing entry
                        it->second.push_back(ref);
                    }
                }
            }
            catch(marisa::Exception const &ex) {
                std::cerr << "Error searching for text: ";
                std::cerr << ex.what() << std::endl;
                return false;
            }
        }
    }

    return true;
}

void TextSearch::splitSearchResult(std::string const &result,
                                   std::string &text,
                                   ObjectFileRef &ref) const
{
    // Get the index that marks the end of the
    // the text and where the FileOffset begins

    // Each result has only one offset that
    // occupies the last 8 bytes of the string,
    // and the offset is in MSB left to right

    FileOffset offset=0;
    FileOffset add=0;
    size_t idx=result.size()-1;
    for(size_t i=0; i < 8; i++) {
        add = (unsigned char)(result[idx]);
        offset |= (add << (i*8));

        idx--;
    }

    // Immediately preceding the FileOffset is
    // a single byte that denotes offset type
    RefType reftype=static_cast<RefType>((unsigned char)(result[idx]));

    ref.Set(offset,reftype);
    text=result.substr(0,idx);
}





}
