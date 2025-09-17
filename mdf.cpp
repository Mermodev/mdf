#include<bits/stdc++.h>
using namespace std;
namespace fs = filesystem;

struct _Color{int Key;string Name,Hex;};
struct _Tag{int Key;string Name,Sign;int Priority;};
struct _FileTag{int TagKey,ColorKey;bool AutoMarked;};
struct _AutoMark{string Name,TagName,ColorName,Extension;};

class MDFManager{
private:
    map<int,_Color> Colors;map<string,int> ColorNameToKey;
    map<int,_Tag> Tags;map<string,int> TagNameToKey;
    map<string,vector<_FileTag>> FileTags;
    vector<_AutoMark> AutoMarks;
    string CurrentDir=fs::current_path().string(),ConfigDir=string(getenv("HOME"))+"/.mdf";
    int NextColorKey=1,NextTagKey=1;
    vector<string> AutoMarkMessages;
    
    void LoadConfig(){
        fs::create_directories(ConfigDir);
        ifstream colors_in(ConfigDir+"/colors.cfg"),tags_in(ConfigDir+"/tags.cfg"),filetags_in(ConfigDir+"/file_tags.cfg"),keys_in(ConfigDir+"/keys.cfg"),automarks_in(ConfigDir+"/automarks.cfg");
        int k;string n,h;while(colors_in>>k>>n>>h){Colors[k]={k,n,h};ColorNameToKey[n]=k;NextColorKey=max(NextColorKey,k+1);}
        int tk;string tn,ts;int tp;while(tags_in>>tk>>tn>>ts>>tp){if(ts=="~^~")ts=" ";Tags[tk]={tk,tn,ts,tp};TagNameToKey[tn]=tk;NextTagKey=max(NextTagKey,tk+1);}
        string p;int tkey,ckey;bool am;while(filetags_in>>p>>tkey>>ckey>>am)FileTags[p].push_back({tkey,ckey,am});
        string amn,atn,acn,aext;while(automarks_in>>amn>>atn>>acn>>aext)AutoMarks.push_back({amn,atn,acn,aext});
        if(keys_in>>NextColorKey>>NextTagKey){}
    }
    
    void SaveConfig(){
        ofstream colors_out(ConfigDir+"/colors.cfg"),tags_out(ConfigDir+"/tags.cfg"),filetags_out(ConfigDir+"/file_tags.cfg"),keys_out(ConfigDir+"/keys.cfg"),automarks_out(ConfigDir+"/automarks.cfg");
        for(auto&[k,c]:Colors)colors_out<<c.Key<<" "<<c.Name<<" "<<c.Hex<<"\n";
        for(auto&[k,t]:Tags)tags_out<<t.Key<<" "<<t.Name<<" "<<((t.Sign!=" ")?t.Sign:"~^~")<<" "<<t.Priority<<"\n";
        for(auto&[p,ft]:FileTags)for(auto&t:ft)filetags_out<<p<<" "<<t.TagKey<<" "<<t.ColorKey<<" "<<t.AutoMarked<<"\n";
        for(auto&am:AutoMarks)automarks_out<<am.Name<<" "<<am.TagName<<" "<<am.ColorName<<" "<<am.Extension<<"\n";
        keys_out<<NextColorKey<<" "<<NextTagKey<<"\n";
    }

    string HexToANSI(const string&h){
        if(h.empty())return"\033[0m";
        string clean=h;if(clean[0]=='#')clean=clean.substr(1);
        if(clean.length()!=6)return"\033[0m";
        int r=stoi(clean.substr(0,2),0,16),g=stoi(clean.substr(2,2),0,16),b=stoi(clean.substr(4,2),0,16);
        return"\033[38;2;"+to_string(r)+";"+to_string(g)+";"+to_string(b)+"m";
    }

    const _FileTag* GetHighestTag(const string&p){
        auto it=FileTags.find(p);if(it==FileTags.end())return nullptr;
        sort(it->second.begin(),it->second.end(),[this](auto&a,auto&b){
            return Tags[a.TagKey].Priority!=Tags[b.TagKey].Priority?Tags[a.TagKey].Priority>Tags[b.TagKey].Priority:
                   Colors[a.ColorKey].Hex!=Colors[b.ColorKey].Hex?Colors[a.ColorKey].Hex<Colors[b.ColorKey].Hex:
                   Tags[a.TagKey].Name<Tags[b.TagKey].Name;
        });
        return it->second.empty()?nullptr:&it->second[0];
    }

    string AbsPath(const string&p){return p.empty()?CurrentDir:(p[0]=='/'?p:(fs::path(CurrentDir)/p).string());}
    
    string ShortPath(){
        vector<string>parts;fs::path p(CurrentDir);
        for(auto&part:p)parts.push_back(part.string());
        return parts.size()<=2?CurrentDir:(parts[parts.size()-2]+"/"+parts.back());
    }

    void AutoMarkFile(const string&path){
        string ext=fs::path(path).extension().string();
        if(ext.empty()&&fs::is_directory(path))ext="/";
        
        // Remove any auto-marked tags that no longer have matching rules
        auto it=FileTags.find(path);
        if(it!=FileTags.end()){
            it->second.erase(remove_if(it->second.begin(),it->second.end(),[&](auto&ft){
                if(ft.AutoMarked){
                    bool rule_exists=false;
                    for(auto&am:AutoMarks){
                        if(am.Extension==ext&&TagNameToKey[am.TagName]==ft.TagKey&&ColorNameToKey[am.ColorName]==ft.ColorKey){
                            rule_exists=true;break;
                        }
                    }
                    return !rule_exists;
                }
                return false;
            }),it->second.end());
            if(it->second.empty())FileTags.erase(it);
        }
        
        // Apply new auto-mark rules
        for(auto&am:AutoMarks){
            if(am.Extension==ext&&TagNameToKey.find(am.TagName)!=TagNameToKey.end()&&ColorNameToKey.find(am.ColorName)!=ColorNameToKey.end()){
                int tkey=TagNameToKey[am.TagName],ckey=ColorNameToKey[am.ColorName];
                bool already_marked=false;
                if(FileTags.find(path)!=FileTags.end()){
                    for(auto&ft:FileTags[path]){
                        if(ft.TagKey==tkey&&ft.ColorKey==ckey){
                            already_marked=true;ft.AutoMarked=true;break;
                        }
                    }
                }
                if(!already_marked){
                    FileTags[path].push_back({tkey,ckey,true});
                    AutoMarkMessages.push_back("\033[33mAuto-marked: "+path+" -> "+am.TagName+":"+am.ColorName+"\033[0m");
                }
                break;
            }
        }
    }

    void TreeRecursive(const string&d,int depth,bool last[]){
        vector<string>entries;for(auto&e:fs::directory_iterator(d)){
            string e_name=e.path().filename().string();if(e_name=="."||e_name=="..")continue;
            string fp=e.path().string();AutoMarkFile(fp);entries.push_back(e_name);
        }
        sort(entries.begin(),entries.end());
        for(size_t i=0;i<entries.size();++i){
            string e=entries[i];
            string fp=(fs::path(d)/e).string();
            for(int j=0;j<depth;++j)cout<<(last[j]?"    ":"│   ");
            cout<<(i==entries.size()-1?"└── ":"├── ");
            auto ht=GetHighestTag(fp);
            if(ht&&Tags.find(ht->TagKey)!=Tags.end()&&Colors.find(ht->ColorKey)!=Colors.end()){
                cout<<HexToANSI(Colors[ht->ColorKey].Hex)<<Tags[ht->TagKey].Sign<<" "<<e<<"\033[0m";
            }else cout<<e;
            auto it=FileTags.find(fp);if(it!=FileTags.end()&&!it->second.empty()){
                cout<<" -> ";for(auto&ft:it->second)cout<<HexToANSI(Colors[ft.ColorKey].Hex)<<Tags[ft.TagKey].Sign<<Tags[ft.TagKey].Name<<"\033[0m ";
            }
            cout<<"\n";
            if(fs::is_directory(fp)){bool newlast[20];copy(last,last+depth,newlast);newlast[depth]=i==entries.size()-1;TreeRecursive(fp,depth+1,newlast);}
        }
    }

    bool IsColorUsed(int key){for(auto&[p,ft]:FileTags)for(auto&t:ft)if(t.ColorKey==key)return true;return false;}
    bool IsTagUsed(int key){for(auto&[p,ft]:FileTags)for(auto&t:ft)if(t.TagKey==key)return true;return false;}

    void ShowAutoMarkMessages(){
        if(!AutoMarkMessages.empty()){
            for(auto&msg:AutoMarkMessages)cout<<msg<<"\n";
            AutoMarkMessages.clear();
        }
    }

public:
    MDFManager(){LoadConfig();}
    ~MDFManager(){SaveConfig();}

    void Touch(string p){
        AutoMarkMessages.clear();
        if(p.empty()){cout<<"\033[31mUsage: touch <file>\033[0m\n";return;}
        string ap=AbsPath(p);
        if(fs::exists(ap)){cout<<"\033[33mFile already exists: "<<ap<<"\033[0m\n";return;}
        
        // Create parent directories if needed
        fs::create_directories(fs::path(ap).parent_path());
        
        ofstream file(ap);
        if(file){cout<<"\033[32mCreated file: "<<ap<<"\033[0m\n";AutoMarkFile(ap);}
        else cout<<"\033[31mFailed to create file: "<<ap<<"\033[0m\n";
        SaveConfig();
        ShowAutoMarkMessages();
    }

    void Mkdir(string p){
        AutoMarkMessages.clear();
        if(p.empty()){cout<<"\033[31mUsage: mkdir <directory>\033[0m\n";return;}
        string ap=AbsPath(p);
        if(fs::exists(ap)){cout<<"\033[33mDirectory already exists: "<<ap<<"\033[0m\n";return;}
        
        if(fs::create_directories(ap)){cout<<"\033[32mCreated directory: "<<ap<<"\033[0m\n";AutoMarkFile(ap);}
        else cout<<"\033[31mFailed to create directory: "<<ap<<"\033[0m\n";
        SaveConfig();
        ShowAutoMarkMessages();
    }

    void Rm(string p){
        AutoMarkMessages.clear();
        if(p.empty()){cout<<"\033[31mUsage: rm <file>\033[0m\n";return;}
        string ap=AbsPath(p);
        if(!fs::exists(ap)){cout<<"\033[31mFile not found: "<<ap<<"\033[0m\n";return;}
        
        if(fs::is_directory(ap)){cout<<"\033[31mUse 'rmdir' for directories: "<<ap<<"\033[0m\n";return;}
        
        if(fs::remove(ap)){
            FileTags.erase(ap);
            cout<<"\033[32mRemoved file: "<<ap<<"\033[0m\n";
        }else cout<<"\033[31mFailed to remove file: "<<ap<<"\033[0m\n";
        SaveConfig();
    }

    void Rmdir(string p){
        AutoMarkMessages.clear();
        if(p.empty()){cout<<"\033[31mUsage: rmdir <directory>\033[0m\n";return;}
        string ap=AbsPath(p);
        if(!fs::exists(ap)){cout<<"\033[31mDirectory not found: "<<ap<<"\033[0m\n";return;}
        
        if(!fs::is_directory(ap)){cout<<"\033[31mNot a directory: "<<ap<<"\033[0m\n";return;}
        
        // Remove tags for all files in directory
        for(auto it=FileTags.begin();it!=FileTags.end();){
            if(it->first.find(ap)==0)it=FileTags.erase(it);
            else ++it;
        }
        
        if(fs::remove_all(ap)>0)cout<<"\033[32mRemoved directory: "<<ap<<"\033[0m\n";
        else cout<<"\033[31mFailed to remove directory: "<<ap<<"\033[0m\n";
        SaveConfig();
    }

    void Copy(string src, string dest){
        AutoMarkMessages.clear();
        if(src.empty() || dest.empty()){cout<<"\033[31mUsage: copy <source> <destination>\033[0m\n";return;}
        
        string src_ap = AbsPath(src);
        string dest_ap = AbsPath(dest);
        
        if(!fs::exists(src_ap)){cout<<"\033[31mSource not found: "<<src_ap<<"\033[0m\n";return;}
        
        // If destination is a directory, copy into it with same filename
        if(fs::is_directory(dest_ap)){
            string filename = fs::path(src_ap).filename().string();
            dest_ap = (fs::path(dest_ap) / filename).string();
        }
        
        if(fs::exists(dest_ap)){cout<<"\033[33mDestination already exists: "<<dest_ap<<"\033[0m\n";return;}
        
        try {
            // Copy the file/directory
            if(fs::is_directory(src_ap)){
                fs::create_directories(dest_ap);
                for(auto& entry : fs::recursive_directory_iterator(src_ap)){
                    auto relative_path = fs::relative(entry.path(), src_ap);
                    auto target_path = fs::path(dest_ap) / relative_path;
                    
                    if(entry.is_directory()){
                        fs::create_directories(target_path);
                    } else {
                        fs::copy_file(entry.path(), target_path);
                    }
                    
                    // Copy tags for this specific path
                    string source_path_str = entry.path().string();
                    string target_path_str = target_path.string();
                    
                    if(FileTags.find(source_path_str) != FileTags.end()){
                        FileTags[target_path_str] = FileTags[source_path_str];
                    }
                }
            } else {
                fs::copy_file(src_ap, dest_ap);
                
                // Copy tags from source to destination
                if(FileTags.find(src_ap) != FileTags.end()){
                    FileTags[dest_ap] = FileTags[src_ap];
                }
            }
            
            cout<<"\033[32mCopied: "<<src_ap<<" -> "<<dest_ap<<"\033[0m\n";
            AutoMarkFile(dest_ap);
            
        } catch(const exception& e) {
            cout<<"\033[31mCopy failed: "<<e.what()<<"\033[0m\n";
        }
        
        SaveConfig();
        ShowAutoMarkMessages();
    }

    void Move(string src, string dest){
        AutoMarkMessages.clear();
        if(src.empty() || dest.empty()){cout<<"\033[31mUsage: move <source> <destination>\033[0m\n";return;}
        
        string src_ap = AbsPath(src);
        string dest_ap = AbsPath(dest);
        
        if(!fs::exists(src_ap)){cout<<"\033[31mSource not found: "<<src_ap<<"\033[0m\n";return;}
        
        // If destination is a directory, move into it with same filename
        if(fs::is_directory(dest_ap)){
            string filename = fs::path(src_ap).filename().string();
            dest_ap = (fs::path(dest_ap) / filename).string();
        }
        
        if(fs::exists(dest_ap)){cout<<"\033[33mDestination already exists: "<<dest_ap<<"\033[0m\n";return;}
        
        try {
            // First copy the file/directory with tags
            if(fs::is_directory(src_ap)){
                fs::create_directories(dest_ap);
                for(auto& entry : fs::recursive_directory_iterator(src_ap)){
                    auto relative_path = fs::relative(entry.path(), src_ap);
                    auto target_path = fs::path(dest_ap) / relative_path;
                    
                    if(entry.is_directory()){
                        fs::create_directories(target_path);
                    } else {
                        fs::copy_file(entry.path(), target_path);
                    }
                    
                    // Copy tags for this specific path
                    string source_path_str = entry.path().string();
                    string target_path_str = target_path.string();
                    
                    if(FileTags.find(source_path_str) != FileTags.end()){
                        FileTags[target_path_str] = FileTags[source_path_str];
                    }
                }
                // Remove the source directory after successful copy
                fs::remove_all(src_ap);
            } else {
                fs::copy_file(src_ap, dest_ap);
                
                // Copy tags from source to destination
                if(FileTags.find(src_ap) != FileTags.end()){
                    FileTags[dest_ap] = FileTags[src_ap];
                }
                // Remove the source file after successful copy
                fs::remove(src_ap);
            }
            
            // Remove source tags after successful move
            if(FileTags.find(src_ap) != FileTags.end()){
                FileTags.erase(src_ap);
            }
            
            cout<<"\033[32mMoved: "<<src_ap<<" -> "<<dest_ap<<"\033[0m\n";
            AutoMarkFile(dest_ap);
            
        } catch(const exception& e) {
            cout<<"\033[31mMove failed: "<<e.what()<<"\033[0m\n";
        }
        
        SaveConfig();
        ShowAutoMarkMessages();
    }

    void Open(string p){
        AutoMarkMessages.clear();
        if(p.empty()){cout<<"\033[31mUsage: open <file>\033[0m\n";return;}
        string ap=AbsPath(p);
        if(!fs::exists(ap)){cout<<"\033[31mFile not found: "<<ap<<"\033[0m\n";return;}
        if(fs::is_directory(ap)){cout<<"\033[31mUse 'cd' for directories: "<<ap<<"\033[0m\n";return;}
        string cmd="nvim \""+ap+"\"";
        int result=system(cmd.c_str());
        if(result==0)cout<<"\033[32mOpened file: "<<ap<<"\033[0m\n";
        else cout<<"\033[31mFailed to open file: "<<ap<<"\033[0m\n";
        AutoMarkFile(ap);
        ShowAutoMarkMessages();
    }

    void CD(string p){
        AutoMarkMessages.clear();
        if(p.empty()){cout<<"\033[31mUsage: cd <directory>\033[0m\n";return;}
        fs::path np=p;if(!np.is_absolute())np=fs::path(CurrentDir)/p;
        if(fs::exists(np)&&fs::is_directory(np))CurrentDir=fs::canonical(np).string();
        else cout<<"\033[31mDirectory not found: "<<p<<"\033[0m\n";
        SaveConfig();
    }

    void Mark(string p,string t,string c){
        AutoMarkMessages.clear();
        if(p.empty()||t.empty()||c.empty()){cout<<"\033[31mUsage: mark <file> <tag> <color>\033[0m\n";return;}
        string ap=AbsPath(p);AutoMarkFile(ap);
        if(TagNameToKey.find(t)==TagNameToKey.end()){cout<<"\033[31mTag '"<<t<<"' not found.\033[0m\n";return;}
        if(ColorNameToKey.find(c)==ColorNameToKey.end()){cout<<"\033[31mColor '"<<c<<"' not found.\033[0m\n";return;}
        int tkey=TagNameToKey[t],ckey=ColorNameToKey[c];
        if([&](){for(auto&ft:FileTags[ap])if(ft.TagKey==tkey)return true;return false;}()){cout<<"\033[33mTag already exists.\033[0m\n";return;}
        FileTags[ap].push_back({tkey,ckey,false});cout<<"\033[32mMarked: "<<ap<<" -> "<<t<<":"<<c<<"\033[0m\n";
        SaveConfig();
        ShowAutoMarkMessages();
    }

    void Unmark(string p,string t){
        AutoMarkMessages.clear();
        if(p.empty()||t.empty()){cout<<"\033[31mUsage: unmark <file> <tag>\033[0m\n";return;}
        string ap=AbsPath(p);AutoMarkFile(ap);
        if(TagNameToKey.find(t)==TagNameToKey.end()){cout<<"\033[31mTag '"<<t<<"' not found.\033[0m\n";return;}
        int tkey=TagNameToKey[t];
        auto it=FileTags.find(ap);if(it==FileTags.end()){cout<<"\033[33mNo tags found.\033[0m\n";return;}
        auto pos=find_if(it->second.begin(),it->second.end(),[&](auto&ft){return ft.TagKey==tkey;});
        if(pos==it->second.end()){cout<<"\033[31mTag not found.\033[0m\n";return;}
        it->second.erase(pos);if(it->second.empty())FileTags.erase(it);cout<<"\033[32mUnmarked: "<<ap<<" - "<<t<<"\033[0m\n";
        SaveConfig();
        ShowAutoMarkMessages();
    }

    void LS(string d="."){
        AutoMarkMessages.clear();
        string ad=d=="."?CurrentDir:(d[0]=='/'?d:(fs::path(CurrentDir)/d).string());
        if(!fs::exists(ad)){cout<<"\033[31mDirectory not found: "<<ad<<"\033[0m\n";return;}
        vector<string>files;for(auto&e:fs::directory_iterator(ad)){
            string fn=e.path().filename().string();if(fn!="."&&fn!="..")files.push_back(fn);
            string fp=(fs::path(ad)/fn).string();AutoMarkFile(fp);
        }
        sort(files.begin(),files.end());
        for(auto&f:files){
            string fp=(fs::path(ad)/f).string();
            auto ht=GetHighestTag(fp);
            if(ht&&Tags.find(ht->TagKey)!=Tags.end()&&Colors.find(ht->ColorKey)!=Colors.end()){
                cout<<HexToANSI(Colors[ht->ColorKey].Hex)<<Tags[ht->TagKey].Sign<<" "<<f<<"\033[0m";
            }else cout<<f;
            auto it=FileTags.find(fp);if(it!=FileTags.end()&&!it->second.empty()){
                cout<<" -> ";for(auto&ft:it->second)cout<<HexToANSI(Colors[ft.ColorKey].Hex)<<Tags[ft.TagKey].Sign<<Tags[ft.TagKey].Name<<"\033[0m ";
            }
            cout<<"\n";
        }
        ShowAutoMarkMessages();
    }

    void Tree(string d="."){
        AutoMarkMessages.clear();
        string ad=d=="."?CurrentDir:(d[0]=='/'?d:(fs::path(CurrentDir)/d).string());
        if(!fs::exists(ad)){cout<<"\033[31mDirectory not found: "<<ad<<"\033[0m\n";return;}
        AutoMarkFile(ad);
        cout<<ad<<"\n";bool last[1]={true};TreeRecursive(ad,0,last);
        ShowAutoMarkMessages();
    }

    void AddTag(string n,string s,int p){
        AutoMarkMessages.clear();
        if(n.empty()||p<0){cout<<"\033[31mUsage: tag add <name> [sign(char)] <priority(int)>\033[0m\n";return;}
        if(TagNameToKey.find(n)!=TagNameToKey.end()){cout<<"\033[31mTag '"<<n<<"' already exists.\033[0m\n";return;}
        if(s.empty())s=" ";else if(s.size()!=1){cout<<"\033[31mSign must be single character or empty\033[0m\n";return;}
        Tags[NextTagKey]={NextTagKey,n,s,p};TagNameToKey[n]=NextTagKey++;
        cout<<"\033[32mTag added: "<<n<<"\033[0m\n";SaveConfig();
    }

    void EditTag(string oldname,string n,string s,int p){
        AutoMarkMessages.clear();
        if(oldname.empty()||n.empty()||p<0){cout<<"\033[31mUsage: tag edit <oldname> <newname> [sign] <priority>\033[0m\n";return;}
        if(TagNameToKey.find(oldname)==TagNameToKey.end()){cout<<"\033[31mTag '"<<oldname<<"' not found.\033[0m\n";return;}
        if(oldname!=n&&TagNameToKey.find(n)!=TagNameToKey.end()){cout<<"\033[31mTag '"<<n<<"' already exists.\033[0m\n";return;}
        if(s.empty())s=" ";else if(s.size()!=1){cout<<"\033[31mSign must be single character or empty\033[0m\n";return;}
        int key=TagNameToKey[oldname];TagNameToKey.erase(oldname);
        Tags[key]={key,n,s,p};TagNameToKey[n]=key;
        cout<<"\033[32mTag edited: "<<oldname<<" -> "<<n<<"\033[0m\n";SaveConfig();
    }

    void RemoveTag(string n){
        AutoMarkMessages.clear();
        if(n.empty()){cout<<"\033[31mUsage: tag remove <name>\033[0m\n";return;}
        if(TagNameToKey.find(n)==TagNameToKey.end()){cout<<"\033[31mTag '"<<n<<"' not found.\033[0m\n";return;}
        int key=TagNameToKey[n];
        if(IsTagUsed(key)){cout<<"\033[31mCannot remove tag '"<<n<<"' - it is in use.\033[0m\n";return;}
        Tags.erase(key);TagNameToKey.erase(n);
        cout<<"\033[32mTag removed: "<<n<<"\033[0m\n";SaveConfig();
    }

    void AddColor(string n,string h){
        AutoMarkMessages.clear();
        if(n.empty()||h.empty()||h[0]!='#'||h.size()!=7){cout<<"\033[31mUsage: color add <name> <#RRGGBB>\033[0m\n";return;}
        if(ColorNameToKey.find(n)!=ColorNameToKey.end()){cout<<"\033[31mColor '"<<n<<"' already exists.\033[0m\n";return;}
        Colors[NextColorKey]={NextColorKey,n,h};ColorNameToKey[n]=NextColorKey++;
        cout<<"\033[32mColor added: "<<n<<"\033[0m\n";SaveConfig();
    }

    void EditColor(string oldname,string n,string h){
        AutoMarkMessages.clear();
        if(oldname.empty()||n.empty()||h.empty()||h[0]!='#'||h.size()!=7){cout<<"\033[31mUsage: color edit <oldname> <newname> <#RRGGBB>\033[0m\n";return;}
        if(ColorNameToKey.find(oldname)==ColorNameToKey.end()){cout<<"\033[31mColor '"<<oldname<<"' not found.\033[0m\n";return;}
        if(oldname!=n&&ColorNameToKey.find(n)!=ColorNameToKey.end()){cout<<"\033[31mColor '"<<n<<"' already exists.\033[0m\n";return;}
        int key=ColorNameToKey[oldname];ColorNameToKey.erase(oldname);
        Colors[key]={key,n,h};ColorNameToKey[n]=key;
        cout<<"\033[32mColor edited: "<<oldname<<" -> "<<n<<"\033[0m\n";SaveConfig();
    }

    void RemoveColor(string n){
        AutoMarkMessages.clear();
        if(n.empty()){cout<<"\033[31mUsage: color remove <name>\033[0m\n";return;}
        if(ColorNameToKey.find(n)==ColorNameToKey.end()){cout<<"\033[31mColor '"<<n<<"' not found.\033[0m\n";return;}
        int key=ColorNameToKey[n];
        if(IsColorUsed(key)){cout<<"\033[31mCannot remove color '"<<n<<"' - it is in use.\033[0m\n";return;}
        Colors.erase(key);ColorNameToKey.erase(n);
        cout<<"\033[32mColor removed: "<<n<<"\033[0m\n";SaveConfig();
    }

    void AddAutoMark(string n,string t,string c,string ext){
        AutoMarkMessages.clear();
        if(n.empty()||t.empty()||c.empty()||ext.empty()){cout<<"\033[31mUsage: automark add <name> <tag> <color> <extension>\033[0m\n";return;}
        if(TagNameToKey.find(t)==TagNameToKey.end()){cout<<"\033[31mTag '"<<t<<"' not found.\033[0m\n";return;}
        if(ColorNameToKey.find(c)==ColorNameToKey.end()){cout<<"\033[31mColor '"<<c<<"' not found.\033[0m\n";return;}
        if(find_if(AutoMarks.begin(),AutoMarks.end(),[&](auto&am){return am.Name==n;})!=AutoMarks.end()){cout<<"\033[31mAutoMark '"<<n<<"' already exists.\033[0m\n";return;}
        AutoMarks.push_back({n,t,c,ext});cout<<"\033[32mAutoMark added: "<<n<<"\033[0m\n";SaveConfig();
    }

    void EditAutoMark(string oldname,string n,string t,string c,string ext){
        AutoMarkMessages.clear();
        if(oldname.empty()||n.empty()||t.empty()||c.empty()||ext.empty()){cout<<"\033[31mUsage: automark edit <oldname> <newname> <tag> <color> <extension>\033[0m\n";return;}
        if(TagNameToKey.find(t)==TagNameToKey.end()){cout<<"\033[31mTag '"<<t<<"' not found.\033[0m\n";return;}
        if(ColorNameToKey.find(c)==ColorNameToKey.end()){cout<<"\033[31mColor '"<<c<<"' not found.\033[0m\n";return;}
        auto it=find_if(AutoMarks.begin(),AutoMarks.end(),[&](auto&am){return am.Name==oldname;});
        if(it==AutoMarks.end()){cout<<"\033[31mAutoMark '"<<oldname<<"' not found.\033[0m\n";return;}
        if(oldname!=n&&find_if(AutoMarks.begin(),AutoMarks.end(),[&](auto&am){return am.Name==n;})!=AutoMarks.end()){cout<<"\033[31mAutoMark '"<<n<<"' already exists.\033[0m\n";return;}
        *it={n,t,c,ext};cout<<"\033[32mAutoMark edited: "<<oldname<<" -> "<<n<<"\033[0m\n";SaveConfig();
    }

    void RemoveAutoMark(string n){
        AutoMarkMessages.clear();
        if(n.empty()){cout<<"\033[31mUsage: automark remove <name>\033[0m\n";return;}
        auto it=find_if(AutoMarks.begin(),AutoMarks.end(),[&](auto&am){return am.Name==n;});
        if(it==AutoMarks.end()){cout<<"\033[31mAutoMark '"<<n<<"' not found.\033[0m\n";return;}
        AutoMarks.erase(it);cout<<"\033[32mAutoMark removed: "<<n<<"\033[0m\n";SaveConfig();
    }

    void ListAutoMarks(){
        AutoMarkMessages.clear();
        cout<<"\033[36mAutoMarks:\033[0m\n";
        for(auto&am:AutoMarks)cout<<"  "<<am.Name<<" -> "<<am.TagName<<":"<<am.ColorName<<" for *"<<am.Extension<<"\n";
    }

    void ListTags(){
        AutoMarkMessages.clear();
        cout<<"\033[36mTags:\033[0m\n";for(auto&[k,t]:Tags)cout<<"  "<<t.Sign<<" "<<t.Name<<" (prio:"<<t.Priority<<")\n";
    }
    
    void ListColors(){
        AutoMarkMessages.clear();
        cout<<"\033[36mColors:\033[0m\n";
        for(auto&[k,c]:Colors){
            int r=stoi(c.Hex.substr(1,2),0,16),g=stoi(c.Hex.substr(3,2),0,16),b=stoi(c.Hex.substr(5,2),0,16);
            cout<<"  \033[38;2;"<<r<<";"<<g<<";"<<b<<"m"<<c.Hex<<"\033[0m "<<c.Name<<"\n";
        }
    }

    void Help(){
        AutoMarkMessages.clear();
        cout<<"\033[1;36mMDF - Mermodev File Manager\033[0m\n";
        cout<<"\033[1;33mFile Operations:\033[0m\n";
        cout<<"  \033[32mls [dir]\033[0m          - List files with tags\n";
        cout<<"  \033[32mtree [dir]\033[0m        - Show directory tree\n";
        cout<<"  \033[32mcd <dir>\033[0m          - Change directory\n";
        cout<<"  \033[32mtouch <file>\033[0m      - Create empty file\n";
        cout<<"  \033[32mmkdir <dir>\033[0m       - Create directory\n";
        cout<<"  \033[32mrm <file>\033[0m         - Remove file\n";
        cout<<"  \033[32mrmdir <dir>\033[0m       - Remove directory\n";
        cout<<"  \033[32mcopy <src> <dest>\033[0m - Copy file/directory (preserves tags)\n";
        cout<<"  \033[32mmove <src> <dest>\033[0m - Move file/directory (preserves tags)\n";
        cout<<"  \033[32mopen <file>\033[0m       - Open file with nvim\n";
        cout<<"  \033[32mmark <file> <tag> <color>\033[0m - Add tag to file\n";
        cout<<"  \033[32munmark <file> <tag>\033[0m - Remove tag from file\n";
        cout<<"\033[1;33mTag Management:\033[0m\n";
        cout<<"  \033[32mtag add <name> [sign] <prio>\033[0m - Add new tag\n";
        cout<<"  \033[32mtag edit <old> <new> [sign] <prio>\033[0m - Edit tag\n";
        cout<<"  \033[32mtag remove <name>\033[0m   - Remove tag\n";
        cout<<"  \033[32mtag list\033[0m           - List all tags\n";
        cout<<"\033[1;33mColor Management:\033[0m\n";
        cout<<"  \033[32mcolor add <name> <#hex>\033[0m - Add new color\n";
        cout<<"  \033[32mcolor edit <old> <new> <#hex>\033[0m - Edit color\n";
        cout<<"  \033[32mcolor remove <name>\033[0m - Remove color\n";
        cout<<"  \033[32mcolor list\033[0m        - List all colors\n";
        cout<<"\033[1;33mAutoMark Management:\033[0m\n";
        cout<<"  \033[32mautomark add <name> <tag> <color> <ext>\033[0m - Add AutoMark rule\n";
        cout<<"  \033[32mautomark edit <old> <new> <tag> <color> <ext>\033[0m - Edit AutoMark rule\n";
        cout<<"  \033[32mautomark remove <name>\033[0m - Remove AutoMark rule\n";
        cout<<"  \033[32mautomark list\033[0m     - List AutoMark rules\n";
        cout<<"\033[1;33mOther:\033[0m\n";
        cout<<"  \033[32mhelp\033[0m             - Show this help\n";
        cout<<"  \033[32mquit\033[0m             - Exit MDF\n";
        cout<<"\033[1;35mStorage: \033[0m"<<"\033[3m"<<ConfigDir<<"\033[0m\n";
    }

    void RunShell(){
        cout<<"\033[1;35mMDF - Mermodev File Manager\033[0m\n";
        cout<<"\033[3mType 'help' for commands, 'quit' to exit\033[0m\n\n";
        string input;
        while(true){
            cout<<"\033[32mmdf\033[0m:\033[34m"<<ShortPath()<<"\033[0m> ";
            getline(cin,input);if(input.empty())continue;
            vector<string>args;istringstream iss(input);string arg;
            while(iss>>arg)args.push_back(arg);
            
            if(args[0]=="quit"||args[0]=="exit")break;
            else if(args[0]=="cd")args.size()==2?CD(args[1]):CD("");
            else if(args[0]=="ls")LS(args.size()>1?args[1]:".");
            else if(args[0]=="tree")Tree(args.size()>1?args[1]:".");
            else if(args[0]=="touch")args.size()==2?Touch(args[1]):Touch("");
            else if(args[0]=="mkdir")args.size()==2?Mkdir(args[1]):Mkdir("");
            else if(args[0]=="rm")args.size()==2?Rm(args[1]):Rm("");
            else if(args[0]=="rmdir")args.size()==2?Rmdir(args[1]):Rmdir("");
            else if(args[0]=="copy")args.size()==3?Copy(args[1],args[2]):Copy("","");
            else if(args[0]=="move")args.size()==3?Move(args[1],args[2]):Move("","");
            else if(args[0]=="open")args.size()==2?Open(args[1]):Open("");
            else if(args[0]=="mark")args.size()==4?Mark(args[1],args[2],args[3]):Mark("","","");
            else if(args[0]=="unmark")args.size()==3?Unmark(args[1],args[2]):Unmark("","");
            else if(args[0]=="tag"&&args.size()>=2){
                if(args[1]=="add")args.size()>=4?AddTag(args[2],args.size()>4?args[3]:"",stoi(args[args.size()>4?4:3])):AddTag("","",-1);
                else if(args[1]=="edit")args.size()>=5?EditTag(args[2],args[3],args.size()>5?args[4]:"",stoi(args[args.size()>5?5:4])):EditTag("","","",-1);
                else if(args[1]=="remove")args.size()==3?RemoveTag(args[2]):RemoveTag("");
                else if(args[1]=="list")ListTags();
                else cout<<"\033[31mInvalid tag command.\033[0m\n";
            }
            else if(args[0]=="color"&&args.size()>=2){
                if(args[1]=="add")args.size()==4?AddColor(args[2],args[3]):AddColor("","");
                else if(args[1]=="edit")args.size()==5?EditColor(args[2],args[3],args[4]):EditColor("","","");
                else if(args[1]=="remove")args.size()==3?RemoveColor(args[2]):RemoveColor("");
                else if(args[1]=="list")ListColors();
                else cout<<"\033[31mInvalid color command.\033[0m\n";
            }
            else if(args[0]=="automark"&&args.size()>=2){
                if(args[1]=="add")args.size()==6?AddAutoMark(args[2],args[3],args[4],args[5]):AddAutoMark("","","","");
                else if(args[1]=="edit")args.size()==7?EditAutoMark(args[2],args[3],args[4],args[5],args[6]):EditAutoMark("","","","","");
                else if(args[1]=="remove")args.size()==3?RemoveAutoMark(args[2]):RemoveAutoMark("");
                else if(args[1]=="list")ListAutoMarks();
                else cout<<"\033[31mInvalid automark command.\033[0m\n";
            }
            else if(args[0]=="help")Help();
            else cout<<"\033[31mUnknown command: "<<args[0]<<"\033[0m\n";
        }
    }
};

int main(int argc,char**argv){
    MDFManager mdf;
    if(argc==1)mdf.RunShell();
    else if(argc==2&&string(argv[1])=="help")cout<<"Run without args for shell mode: mdf\n";
    else cout<<"Use interactive mode: mdf\n";
    return 0;
}
