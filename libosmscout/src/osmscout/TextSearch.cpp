#include <iostream>
#include <osmscout/util/String.h>
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

    TrieInfo trie;
    trie.file=proper_path+"textpoi.dat";
    m_list_tries.push_back(trie);

    trie.file=proper_path+"textloc.dat";
    m_list_tries.push_back(trie);

    trie.file=proper_path+"textregion.dat";
    m_list_tries.push_back(trie);

    trie.file=proper_path+"textother.dat";
    m_list_tries.push_back(trie);

    uint8_t tries_avail=0;
    for(size_t i=0; i < m_list_tries.size(); i++) {
        // open/load the data file
        try {
            tries_avail++;
            m_list_tries[i].avail=true;
            m_list_tries[i].trie = new marisa::Trie;
            m_list_tries[i].trie->load(m_list_tries[i].file.c_str());
        }
        catch(marisa::Exception const &ex) {
            // We don't return false on a failed load attempt
            // since its possible that the user does not want
            // to include a specific trie (ie. textother)
            std::cerr << "Warn, could not open " << m_list_tries[i].file << ":";
            std::cerr << ex.what() << std::endl;
            delete m_list_tries[i].trie;
            m_list_tries[i].avail=false;
            tries_avail--;
        }
    }

    if(tries_avail==0) {
        std::cerr << "TextSearch: No valid text data files available";
        return false;
    }

    // Determine the number of bytes used for offsets
    for(size_t i=0; i < m_list_tries.size(); i++) {
        if(m_list_tries[i].avail) {
            // We use an ASCII control character to denote
            // the start of the sz offset key:
            // 0x04: EOT
            std::string sz_offset_query;
            sz_offset_query.push_back(4);

            marisa::Agent agent;
            agent.set_query(sz_offset_query.c_str(),
                            sz_offset_query.length());

            // there should only be one result
            if(m_list_tries[i].trie->predictive_search(agent)) {
                std::string result(agent.key().ptr(),agent.key().length());
                result.erase(0,1);  // get rid of the ASCII control char
                if(!StringToNumberUnsigned(result,m_sz_offset)) {
                    std::cerr << "Could not parse file offset size in text data";
                    return false;
                }
                break;
            }
            else {
                std::cerr << "Could not find file offset size in text data";
                return false;
            }
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
        if(list_search_groups[i] && m_list_tries[i].avail) {
            marisa::Agent agent;

            try {
                agent.set_query(query.c_str(),query.length());
                while(m_list_tries[i].trie->predictive_search(agent)) {
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

    // Each result has only one offset that occupies
    // the last m_sz_offset bytes of the string, and
    // the offset is in MSB left to right

    FileOffset offset=0;
    FileOffset add=0;
    size_t idx=result.size()-1;
    for(size_t i=0; i < m_sz_offset; i++) {
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
