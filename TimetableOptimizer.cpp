// instead of generating all feasible placements we can simply compute feasible rooms
// since we can always put the lecture in these rooms.
#include <bits/stdc++.h>
using namespace std;

#ifndef ONLINE_JUDGE
#define debug(x) cerr << #x << " : " << x << endl;
#define TIME_START auto start = chrono::high_resolution_clock::now();
#define TIME_END \
    auto end = chrono::high_resolution_clock::now(); \
    cerr << "Time: " \
         << chrono::duration_cast<chrono::milliseconds>(end - start).count() \
         << " ms\n";
#else
#define debug(x)
#define TIME_START
#define TIME_END
#endif

// class/functions names in PascalCase
// members in class with camelCase
class Professor{
public:
    int id;
    string code;
    string name;
};

class Room{
public:
    int id;
    string name;
    int capacity;
};

class StudentGroup{
public:
    int id;
    string name;
    int strength;
};

class Course{
public:
    int id;
    string courseName;
    int professorId;
    int studentGroupId;
    int lecturesPerWeek;
};

class Lecture{
public:
    int id;
    int courseId;
    int roomId;
    int day;
    int slot;
};

struct Placement{
    int day;
    int slot;
    int roomId;
};

enum MoveType{
    SHIFT,
    SWAP
};

struct Move{
    // this is beautiful to generate neighbor time table;
    int lectureId;
    int swapLectureId;// added in the last, for swap;
    MoveType moveType;
    Placement oldPlacement;
    Placement newPlacement;
};

class Scheduler{
    mt19937 rng;
    const int STUDENT_GAP_PENALTY = 5;
    const int PROFESSOR_GAP_PENALTY = 6;
    const int CONSECUTIVE_PENALTY = 4;
    const int MORNING_SLOT_PENALTY = 3;
    const int LAST_SLOT_PENALTY = 1;
    const int EXTRA_CLASS_PENALTY = 3;
    const int AFTER_LUNCH_PENALTY=2;
public:
    Scheduler(){
        rng.seed(chrono::steady_clock::now().time_since_epoch().count());
    }
    int noOfProf,noOfRooms,noOfStudentGroups,noOfCourses;
    // input data useful for output
    vector<Lecture> lectures;
    vector<Course> courses;
    vector<Room> rooms;
    vector<Professor> professors;
    vector<StudentGroup> studentGroups;

    // Group wise time tables for fast penality calculaions and printing
    vector<vector<vector<int>>> studentTable;
    vector<vector<vector<int>>> profTable;
    vector<vector<vector<int>>> roomTable;

    // codes to input index mapper
    unordered_map<string, int> professorIdMap;
    unordered_map<string, int> roomIdMap;
    unordered_map<string, int> studentGroupIdMap;

    // occupancy tables for quick checking
    vector<vector<vector<int>>> roomOccupied;
    vector<vector<vector<int>>> professorOccupied;
    vector<vector<vector<int>>> groupOccupied;
    
    // statically feasible 
    vector<vector<int>> feasibleRooms;

    //---------------------Input----------------------------------------
    void InputProfessors(){
        cout<<"Input number of professors:\n";
        cin>>noOfProf;
        cout<<"Input the professors Names and codes one after other:\n";
        for(int i=0; i<noOfProf; i++){
            Professor curr_prof;
            cin>>curr_prof.name>>curr_prof.code;
            professorIdMap[curr_prof.code]=i;
            curr_prof.id=i;
            professors.push_back(curr_prof);
        }
        return;
    }

    void InputRooms(){
        cout<<"Input number of rooms:\n";
        cin>>noOfRooms;
        cout<<"Input all names along with room capcaty one after other:\n";
        for(int i=0; i<noOfRooms; i++){
            Room curr_room;
            cin>>curr_room.name>>curr_room.capacity;
            roomIdMap[curr_room.name]=i;
            curr_room.id=i;
            rooms.push_back(curr_room);
        }
        return;
    }

    void InputStudentGroups(){
        cout<<"Input number of student groups/departments:\n";
        cin>>noOfStudentGroups;
        cout<<"Input the name of the student group and their strength :\n";
        for(int i=0; i<noOfStudentGroups; i++) {
            StudentGroup sg;
            cin>>sg.name>>sg.strength;
            sg.id=i;
            studentGroupIdMap[sg.name]=i;
            studentGroups.push_back(sg);
        }
        return;
    }

    void InputCourses(){
        cout<<"Input number of courses:\n";
        cin>>noOfCourses;
        cout<<"Input the course name, professor id, student group id, lectures per week:\n";
        string professorCode, studentsCode;
        for(int i=0; i<noOfCourses; i++) {
            Course curr_course;
            cin>>curr_course.courseName>>professorCode>>studentsCode>>curr_course.lecturesPerWeek;
            curr_course.id=i;
            curr_course.professorId=professorIdMap[professorCode];
            curr_course.studentGroupId=studentGroupIdMap[studentsCode];
            courses.push_back(curr_course);
        }
        return;
    }

    //---------------------Generation----------------------------------------
    void ScheduleInitialiser(){
        generateLectures();
        InitialiseOccupancyTables();
        FillFeasibleRooms();
    }
    void generateLectures(){
        int idx=0;
        for(int i=0;i<noOfCourses;i++){
            for(int j=0;j<courses[i].lecturesPerWeek; j++){
                Lecture curr_lecture;
                curr_lecture.id=idx;
                curr_lecture.courseId=courses[i].id;
                curr_lecture.roomId=-1;
                curr_lecture.day=-1;
                curr_lecture.slot=-1;
                idx++;
                lectures.push_back(curr_lecture);
            }
        }
    }

    // New addition;
    void FillFeasibleRooms(){
        for(int lectureId=0;lectureId<lectures.size();lectureId++){
            vector<int> feasibleRoomIds;
            int currStrength=studentGroups[courses[lectures[lectureId].courseId].studentGroupId].strength;
            for(int room=0;room<noOfRooms;room++){
                if(rooms[room].capacity>=currStrength)feasibleRoomIds.push_back(room);
            }
            feasibleRooms.push_back(feasibleRoomIds);
        }
    }

    void InitialiseOccupancyTables(){
        roomOccupied.assign(5,vector<vector<int>>(9,vector<int>(noOfRooms,-1)));
        professorOccupied.assign(5,vector<vector<int>>(9,vector<int>(noOfProf,-1)));
        groupOccupied.assign(5,vector<vector<int>>(9,vector<int>(noOfStudentGroups,-1)));
        studentTable.assign(noOfStudentGroups,vector<vector<int>>(5,vector<int>(9,-1)));
        profTable.assign(noOfProf,vector<vector<int>>(5,vector<int>(9,-1)));
        roomTable.assign(noOfRooms,vector<vector<int>>(5,vector<int>(9,-1)));
    }
    //--------------------validating functions-------------------

    bool CanAssign(int lectureId,const Placement& placement){
        int day=placement.day;
        int slot=placement.slot;
        int roomId=placement.roomId;
        if(roomOccupied[day][slot][roomId]!=-1)return false;

        Lecture &lecture=lectures[lectureId];
        int profId=courses[lecture.courseId].professorId;
        if(professorOccupied[day][slot][profId]!=-1)return false;

        int stuId=courses[lecture.courseId].studentGroupId;
        int total=studentGroups[stuId].strength;
        if(groupOccupied[day][slot][stuId]!=-1)return false;
        if(rooms[roomId].capacity < total)return false;
        return true;
    }

    void AssignLecture(int lectureId,const Placement& placement){
        // initially these were also parameters repeatedly taking them so using a struct
        int day=placement.day;
        int slot=placement.slot;
        int roomId=placement.roomId;
        //assert(CanAssign(lectureId, roomId, day, slot));

        roomOccupied[day][slot][roomId]=lectureId;

        Lecture &lecture=lectures[lectureId];
        int profId=courses[lecture.courseId].professorId;
        professorOccupied[day][slot][profId]=lectureId;

        int stuId=courses[lecture.courseId].studentGroupId;
        groupOccupied[day][slot][stuId]=lectureId;

        lecture.roomId=roomId;
        lecture.day=day;
        lecture.slot=slot;

        studentTable[stuId][day][slot]=lectureId;
        profTable[profId][day][slot]=lectureId;
        roomTable[roomId][day][slot]=lectureId;
    }

    void RemoveLecture(int lectureId){

        Lecture &lecture=lectures[lectureId];
        int roomId=lecture.roomId;
        int day=lecture.day;
        int slot=lecture.slot;
        if(lecture.roomId==-1)return;// so that we do not run into runtime errors 
        // lecture is not assigned initially.
        roomOccupied[day][slot][roomId]=-1;

        int profId=courses[lecture.courseId].professorId;
        professorOccupied[day][slot][profId]=-1;

        int stuId=courses[lecture.courseId].studentGroupId;
        groupOccupied[day][slot][stuId]=-1;

        lecture.roomId=-1;
        lecture.day=-1;
        lecture.slot=-1;
        
        studentTable[stuId][day][slot]=-1;
        profTable[profId][day][slot]=-1;
        roomTable[roomId][day][slot]=-1;
    }

    //------------------Initial feasible solution---------------------------
    //----Randomised Greedy----------------------
    vector<Placement> FindFeasiblePlacements(int lectureId){
        vector<Placement> feasiblePlacements;
        for(int day=0;day<5;day++){
            for(int slot=0;slot<9;slot++){
                for(int roomId=0;roomId<noOfRooms;roomId++){
                    Placement currPlacement;
                    currPlacement.day=day;
                    currPlacement.slot=slot;
                    currPlacement.roomId=roomId;
                    if(CanAssign(lectureId,currPlacement))feasiblePlacements.push_back(currPlacement);
                }
            }
        }
        return feasiblePlacements;
    }

    bool RandomPickAssign(int lectureId){
        uniform_int_distribution<int> forDay(0,4);
        uniform_int_distribution<int> forSlot(0,8);
        uniform_int_distribution<int> forRoom(0,noOfRooms-1);
        for(int attempts=0;attempts<100;attempts++){
            Placement placement;
            placement.day=forDay(rng);
            placement.slot=forSlot(rng);
            placement.roomId=forRoom(rng);
            if(CanAssign(lectureId,placement)){
                AssignLecture(lectureId,placement);
                return true;
            }
        }
        return false;
    }

    void ClearTimeTable(){
        for(int i=0;i<lectures.size();i++){
            RemoveLecture(i);
        }
    }

    bool GenerateRandomTimeTable(){

        ClearTimeTable(); 
        // instead of backtracking we can clear and start fresh one and
        // hope we can get a fesible solution.
        vector<int> randomOrder;
        for(int i=0;i<lectures.size();i++)randomOrder.push_back(i);

        shuffle(randomOrder.begin(), randomOrder.end(), rng);
        // to make it more useful  
        // get a useful initial time table

        // Instead we can sort the the lectures based on feasible rooms 
        // and assign them first so that initial solution is found
        // else we would randomly allocate and will not able to find a slot for
        // lectures with less feasible rooms;
        // sort(randomOrder.begin(), randomOrder.end(),
        //      [&](int a,int b){

        //     if(feasibleRooms[a].size()!=feasibleRooms[b].size())
        //         return feasibleRooms[a].size()<feasibleRooms[b].size();

        //     int la=courses[lectures[a].courseId].lecturesPerWeek;
        //     int lb=courses[lectures[b].courseId].lecturesPerWeek;

        //     return la>lb;
        // });

        for(int i=0;i<lectures.size();i++){
            bool assigned=RandomPickAssign(randomOrder[i]);
            if(assigned)continue;
            vector<Placement> feasiblePlacements=FindFeasiblePlacements(randomOrder[i]);
            if(feasiblePlacements.size()==0)return false;
            shuffle(feasiblePlacements.begin(),feasiblePlacements.end(),rng);
            AssignLecture(randomOrder[i],feasiblePlacements[0]);
        }
        return true;
    }
    //--------------------Print-------------------------
    void PrintLectures(){
        for(auto &lecture : lectures){
            Course &c = courses[lecture.courseId];
            cout<<lecture.id<<" "<<c.courseName<<" "<<professors[c.professorId].name<<" "<<professors[c.professorId].code<<"\n";
        }
    }

    void PrintTimeTable(){
        vector<Lecture> copy=lectures;
        sort(copy.begin(),copy.end(),[](const Lecture &a,const Lecture &b){
            if(a.day!=b.day)return a.day<b.day;
            return a.slot<b.slot;
        });
        // sort based day and slot so that we can instantly see the same times and see whether there is a collision

        cout<<"Current Time Table:\n";
        cout<<"Course Name, Day, slot, room code, Professor, student Group:\n";
        for(int i=0;i<lectures.size();i++){
            int courseId=copy[i].courseId;
            cout<<courses[courseId].courseName<<" Day:"<<copy[i].day<<" Slot:"
            <<copy[i].slot<<" "<<rooms[copy[i].roomId].name<<" "<<professors[courses[courseId].professorId].code<<" "<<studentGroups[courses[courseId].studentGroupId].name<<"\n";
        }
    }

    void PrintDeptTimeTables(){
        vector<string> slotNames={"08-09","09-10","10-11","11-12","12-01","02-03","03-04","04-05","05-06"};
        vector<string> dayNames={"Mon:","Tue:","Wed:","Thu:","Fri:"};
        cout<<left;
        for(int i=0;i<noOfStudentGroups;i++){
            cout<<"--------------------------------------------------------------------------------\n";
            cout<<studentGroups[i].name<<"\n";
            cout<<"     ";
            for(int slot=0;slot<9;slot++)cout<<slotNames[slot]<<" ";
            cout<<"\n";
            for(int day=0;day<5;day++){
                cout<<dayNames[day]<<" ";
                for(int slot=0;slot<9;slot++){
                    if(studentTable[i][day][slot]==-1){
                        cout<<"----- ";
                        continue;
                    }
                    string courseName=courses[lectures[studentTable[i][day][slot]].courseId].courseName;
                    cout<<setw(5)<<courseName<<" ";
                }
                cout<<"\n";
            }
        }
    }

    void PrintProfTimeTables(){
        vector<string> slotNames={"08-09","09-10","10-11","11-12","12-01","02-03","03-04","04-05","05-06"};
        vector<string> dayNames={"Mon:","Tue:","Wed:","Thu:","Fri:"};
        cout<<left;
        for(int i=0;i<noOfProf;i++){
            cout<<"--------------------------------------------------------------------------------\n";
            cout<<professors[i].code<<"\n";
            cout<<"     ";
            for(int slot=0;slot<9;slot++)cout<<slotNames[slot]<<" ";
            cout<<"\n";
            for(int day=0;day<5;day++){
                cout<<dayNames[day]<<" ";
                for(int slot=0;slot<9;slot++){
                    if(profTable[i][day][slot]==-1){
                        cout<<"----- ";
                        continue;
                    }
                    string courseName=courses[lectures[profTable[i][day][slot]].courseId].courseName;
                    cout<<setw(5)<<courseName<<" ";
                }
                cout<<"\n";
            }
        }
    }

    void PrintRoomTimeTables(){
        vector<string> slotNames={"08-09","09-10","10-11","11-12","12-01","02-03","03-04","04-05","05-06"};
        vector<string> dayNames={"Mon:","Tue:","Wed:","Thu:","Fri:"};
        cout<<left;
        for(int i=0;i<noOfRooms;i++){
            cout<<"--------------------------------------------------------------------------------\n";
            cout<<rooms[i].name<<"\n";
            cout<<"     ";
            for(int slot=0;slot<9;slot++)cout<<slotNames[slot]<<" ";
            cout<<"\n";
            for(int day=0;day<5;day++){
                cout<<dayNames[day]<<" ";
                for(int slot=0;slot<9;slot++){
                    if(roomTable[i][day][slot]==-1){
                        cout<<"----- ";
                        continue;
                    }
                    string courseName=courses[lectures[roomTable[i][day][slot]].courseId].courseName;
                    cout<<setw(5)<<courseName<<" ";
                }
                cout<<"\n";
            }
        }
    }

    // Important penalty functions
    // Gap, last slot and morning slot penalties 
    int GapPenalty(){
        int studentPenalty=0,lastSlot=0,morningSlot=0;
        for(int i=0;i<noOfStudentGroups;i++){
            for(int day=0;day<5;day++){
                int prevSlot=-1;
                for(int slot=0;slot<9;slot++){
                    if(studentTable[i][day][slot]!=-1){
                        if(prevSlot!=-1)studentPenalty+=(slot-prevSlot-1)*STUDENT_GAP_PENALTY;// gap=slot-prevSlot-1
                        prevSlot=slot;
                        if(slot==0)morningSlot++;
                        if(slot==8)lastSlot++;
                    }
                }
            }
        }
        int profPenalty=0;
        for(int i=0;i<noOfProf;i++){
            for(int day=0;day<5;day++){
                int prevSlot=-1;
                for(int slot=0;slot<9;slot++){
                    if(profTable[i][day][slot]!=-1){
                        if(prevSlot!=-1)profPenalty+=(slot-prevSlot-1)*PROFESSOR_GAP_PENALTY;// gap=slot-prevSlot-1, closer for 
                        prevSlot=slot;
                        if(slot==0)morningSlot++;
                        if(slot==8)lastSlot++;
                    }
                }
            }
        }
        int morningPenalty=morningSlot*MORNING_SLOT_PENALTY;
        int lastSlotPenalty=lastSlot*LAST_SLOT_PENALTY;
        return profPenalty+studentPenalty+morningPenalty+lastSlotPenalty;
    }

    //consecutive lecture penalties
    int ConsecutivePenalty(){
        int studentPenalty=0;
        for(int i=0;i<noOfStudentGroups;i++){
            for(int day=0;day<5;day++){
                int cons=0;
                for(int slot=0;slot<9;slot++){
                    if(studentTable[i][day][slot]!=-1){
                        cons++;
                        if(cons>2)studentPenalty+=CONSECUTIVE_PENALTY;
                    }else cons=0;
                }
            }
        }
        int profPenalty=0;
        for(int i=0;i<noOfProf;i++){
            for(int day=0;day<5;day++){
                int cons=0;
                for(int slot=0;slot<9;slot++){
                    if(profTable[i][day][slot]!=-1){
                        cons++;
                        if(cons>2)profPenalty+=CONSECUTIVE_PENALTY;
                    }else cons=0;
                }
            }
        }
        return profPenalty+studentPenalty;
    }

    int ClassPerDayPenalty(){
        int studentPenalty=0;
        for(int i=0;i<noOfStudentGroups;i++){
            for(int day=0;day<5;day++){
                int classPerday=0;
                for(int slot=0;slot<9;slot++){
                    if(studentTable[i][day][slot]!=-1){
                        classPerday++;
                        if(classPerday>4)studentPenalty+=(classPerday-4)*EXTRA_CLASS_PENALTY;
                    }
                }
            }
        }
        int profPenalty=0;
        for(int i=0;i<noOfProf;i++){
            for(int day=0;day<5;day++){
                int classPerday=0;
                for(int slot=0;slot<9;slot++){
                    if(profTable[i][day][slot]!=-1){
                        classPerday++;
                        if(classPerday>4)profPenalty+=(classPerday-4)*EXTRA_CLASS_PENALTY;
                    }
                }
            }
        }
        return profPenalty+studentPenalty;
    }

    int AfterLunchPenalty(){
        int studentPenalty=0;
        for(int i=0;i<noOfStudentGroups;i++){
            for(int day=0;day<5;day++){
                int afterLunch=0;
                for(int slot=0;slot<9;slot++){
                    if(studentTable[i][day][slot]!=-1){
                        if(slot>4){
                            afterLunch++;
                            studentPenalty+=afterLunch*AFTER_LUNCH_PENALTY;
                        }
                    }
                }
            }
        }
        int profPenalty=0;
        for(int i=0;i<noOfProf;i++){
            for(int day=0;day<5;day++){
                int afterLunch=0;
                for(int slot=0;slot<9;slot++){
                    if(profTable[i][day][slot]!=-1){
                        if(slot>4){
                            afterLunch++;
                            profPenalty+=afterLunch*AFTER_LUNCH_PENALTY;
                        }
                    }
                }
            }
        }
        return profPenalty+studentPenalty;
    }

    int TotalPenalty(){
        return GapPenalty()+ConsecutivePenalty()+ClassPerDayPenalty()+AfterLunchPenalty();
    }

    bool CanSwap(int lectureId, Placement &oldPlacement, Placement& newPlacement){
        int swapLectureId=roomOccupied[newPlacement.day][newPlacement.slot][newPlacement.roomId];
        RemoveLecture(swapLectureId);

        bool canSwap=true;
        if(!CanAssign(lectureId,newPlacement) || !CanAssign(swapLectureId,oldPlacement))canSwap=false;
        AssignLecture(swapLectureId,newPlacement);

        return canSwap;
    }

    // Neighbor generation;
    Move GenerateNeighbor(){

        uniform_int_distribution<int> forlecture(0,lectures.size()-1);
        int lectureId=forlecture(rng);

        Placement oldPlacement;
        oldPlacement.day=lectures[lectureId].day;
        oldPlacement.slot=lectures[lectureId].slot;
        oldPlacement.roomId=lectures[lectureId].roomId;

        RemoveLecture(lectureId);//to avoid self collision

        Move move;
        move.lectureId=lectureId;
        move.oldPlacement=oldPlacement;

        vector<int> nxtday={0,1,2,3,4},nxtSlot={0,1,2,3,4,5,6,7,8};
        shuffle(nxtday.begin(),nxtday.end(),rng);
        shuffle(nxtSlot.begin(),nxtSlot.end(),rng);
        shuffle(feasibleRooms[lectureId].begin(),feasibleRooms[lectureId].end(),rng);

        for(int i=0;i<5;i++){
            int day=nxtday[i];
            for(int j=0;j<9;j++){
                for(int k=0;k<feasibleRooms[lectureId].size();k++){
                    int slot=nxtSlot[j];
                    Placement placement;
                    placement.day=day;
                    placement.slot=slot;
                    placement.roomId=feasibleRooms[lectureId][k];
                    if(roomOccupied[day][slot][placement.roomId]==-1){
                        if(CanAssign(lectureId,placement)){
                            move.newPlacement=placement;
                            move.moveType=SHIFT;
                            AssignLecture(lectureId,oldPlacement);
                            return move;
                        }
                    }else{
                        if(CanSwap(lectureId,oldPlacement,placement)){
                            move.newPlacement=placement;
                            move.swapLectureId=roomOccupied[day][slot][placement.roomId];
                            move.moveType=SWAP;
                            AssignLecture(lectureId,oldPlacement);
                            return move;
                        }
                    }
                }
            }
        }
        AssignLecture(lectureId,oldPlacement);
        move.newPlacement=oldPlacement;
        return move;
    }

    // To make implementation look better
    void MakeMove(Move &move){
        if(move.moveType==SWAP){
            RemoveLecture(move.lectureId);
            RemoveLecture(move.swapLectureId);
            AssignLecture(move.lectureId,move.newPlacement);
            AssignLecture(move.swapLectureId,move.oldPlacement);
        }else{
            RemoveLecture(move.lectureId);
            AssignLecture(move.lectureId,move.newPlacement);
        }
    }

    void UndoMove(Move &move){
        if(move.moveType==SWAP){
            RemoveLecture(move.lectureId);
            RemoveLecture(move.swapLectureId);
            AssignLecture(move.lectureId,move.oldPlacement);
            AssignLecture(move.swapLectureId,move.newPlacement);
        }else{
            RemoveLecture(move.lectureId);
            AssignLecture(move.lectureId,move.oldPlacement);
        }
    }

    // Simulated Annealing
    void SimulatedAnnealing(){
        double intialTemperature=1000;
        double minTemperature=0.1;
        double coolingRate=0.995;
        
        int currPenalty=TotalPenalty();
        double temperature=intialTemperature;

        uniform_real_distribution<double> dis(0.0,1.0);

        vector<Lecture> bestLectures=lectures;
        int bestPenalty=currPenalty;

        cout<<"\nInitial Penalty:"<<currPenalty<<"\n";
        int swaps=0,shifts=0,accepted=0,rejected=0;
        while(temperature > minTemperature){
            Move move=GenerateNeighbor();
            MakeMove(move);

            if(move.moveType==SWAP)swaps++;
            else shifts++;
            int nextPenalty=TotalPenalty();
            if(nextPenalty < currPenalty){
                currPenalty=nextPenalty;
                accepted++;
            }else{
                int delta=nextPenalty-currPenalty;
                double probability = exp(-1*delta/temperature);

                double randomNumber = dis(rng);

                if(randomNumber < probability){
                    currPenalty = nextPenalty;
                    accepted++;
                }
                else{
                    rejected++;
                    UndoMove(move);
                }
            }
            if(currPenalty < bestPenalty){
                bestPenalty = currPenalty;
                bestLectures = lectures;
            }
            temperature *= coolingRate;
        }
        cout<<"\nfinal Penalty:"<<bestPenalty<<"\n";
        cout<<"\nShifts:"<<shifts<<"\n\n"<<"Swaps:"<<swaps<<"\n\n";
        cout<<"\nAccepted:"<<accepted<<"\n\n"<<"Rejected:"<<rejected<<"\n\n";
        lectures=bestLectures;
        InitialiseOccupancyTables();
        for(int i=0;i<lectures.size();i++){
            Placement placement;
            placement.day=lectures[i].day;
            placement.slot=lectures[i].slot;
            placement.roomId=lectures[i].roomId;
            AssignLecture(lectures[i].id,placement);
        }
    }

    // backtracking based
    bool GenerateRandomTimeTableBacktracking() {
        ClearTimeTable(); 
        
        vector<int> randomOrder(lectures.size());
        iota(randomOrder.begin(), randomOrder.end(), 0);
        shuffle(randomOrder.begin(), randomOrder.end(), rng);
        
        // Start backtracking from the 0th lecture index
        return InitializeWithBacktracking(0, randomOrder);
    }
    bool InitializeWithBacktracking(int lectureIdx, vector<int>& order) {
        // Base Case: All lectures successfully assigned a valid initial slot!
        if (lectureIdx == order.size()) return true;

        int currentLectureId = order[lectureIdx];
        
        // Get all valid placements for this lecture given the CURRENT matrix state
        vector<Placement> choices = FindFeasiblePlacements(currentLectureId);
        
        // Crucial optimization: Try rooms/slots randomly to maintain stochastic variation
        shuffle(choices.begin(), choices.end(), rng);

        for (const auto& placement : choices) {
            AssignLecture(currentLectureId, placement);
            
            // Recurse to place the next lecture
            if (InitializeWithBacktracking(lectureIdx + 1, order)) {
                return true; 
            }
            
            // BACKTRACK: If the downstream path failed, undo this assignment and try the next slot
            RemoveLecture(currentLectureId);
        }

        return false; // Triggers backtracking in the previous stack frame
    }

}; 

int main() {

    Scheduler tt;
    tt.InputProfessors();
    tt.InputRooms();
    tt.InputStudentGroups();
    tt.InputCourses();
    tt.ScheduleInitialiser(); 
    // for stats:
    int totalRuns = 30;
    double totalInitialPenalty = 0;
    double totalFinalPenalty = 0;
    long long totalDurationMs = 0;

    for (int run = 0; run < totalRuns; run++) {
        if (!tt.GenerateRandomTimeTable()) { run--; continue; } // Retry if initial generation fails

        auto start = chrono::high_resolution_clock::now();
        
        totalInitialPenalty += tt.TotalPenalty();
        tt.SimulatedAnnealing();
        totalFinalPenalty += tt.TotalPenalty();
        
        auto end = chrono::high_resolution_clock::now();
        totalDurationMs += chrono::duration_cast<chrono::milliseconds>(end - start).count();
    }

    // Print your CV-ready metrics!
    cout << "\n=== CV BENCHMARK RESULTS ===\n";
    cout << "Average Execution Time: " << (double)totalDurationMs / totalRuns << " ms\n";
    cout << "Average Penalty Reduction: " << ((totalInitialPenalty - totalFinalPenalty) / totalInitialPenalty) * 100 << "%\n";
    // while(!tt.GenerateRandomTimeTable());// bug if time table not 
    // tt.GenerateRandomTimeTableBacktracking();
    // tt.PrintTimeTable();
    // cout<<"\nBefore:\n";
    // tt.PrintDeptTimeTables();
    // tt.PrintProfTimeTables();
    // tt.PrintRoomTimeTables();
    // tt.SimulatedAnnealing();
    // tt.PrintTimeTable();
    // cout<<"\nAfter:\n";
    // tt.PrintDeptTimeTables();
    // tt.PrintProfTimeTables();
    // tt.PrintRoomTimeTables();

    return 0;
}
/*
Input :
5
hell P01
hello P02
worl P03
world P04
come P05
5
NR11 250
NR12 250
NR13 150
NR14 50
NR15 25
3
Mech 240
Cse 120
Ece 140
8
DAA P01 Mech 3
DAA P02 Cse 3
ET P04 Ece 4
PDS P05 Cse 3
ML P02 Mech 2
Ds P03 Cse 2
POW P03 Mech 4
EC P02 Ece 4
*/
/*
12
Alice P01
Bob P02
Charlie P03
David P04
Emma P05
Frank P06
Grace P07
Henry P08
Irene P09
Jack P10
Kevin P11
Laura P12

8
NR101 250
NR102 220
NR103 180
NR104 150
NR105 120
NR106 100
NR107 80
NR108 60

3
CSE 180
ECE 140
ME 220

24
DAA P01 CSE 4
DBMS P02 CSE 3
OS P03 CSE 4
CN P04 CSE 3
AI P05 CSE 3
ML P06 CSE 4
DS P07 CSE 3
SE P08 CSE 2

SIGN P09 ECE 4
DSP P10 ECE 4
NET P11 ECE 3
VLSI P12 ECE 4
EMFT P03 ECE 3
CTRL P04 ECE 3
MICR P05 ECE 2
COMM P06 ECE 3

THER P07 ME 4
SOM P08 ME 3
DYN P09 ME 4
MANF P10 ME 3
FLUD P11 ME 4
MDES P12 ME 4
HEAT P01 ME 3
CAD P02 ME 3
*/
/*
15
P1 P01
P2 P02
P3 P03
P4 P04
P5 P05
P6 P06
P7 P07
P8 P08
P9 P09
P10 P10
P11 P11
P12 P12
P13 P13
P14 P14
P15 P15
5
R1 250
R2 220
R3 180
R4 150
R5 120
3
CSE 180
ECE 150
ME 220
36
C01 P01 CSE 4
C02 P02 CSE 4
C03 P03 CSE 3
C04 P04 CSE 4
C05 P05 CSE 3
C06 P06 CSE 4
C07 P07 CSE 3
C08 P08 CSE 3
C09 P09 CSE 4
C10 P10 CSE 3
C11 P11 CSE 3
C12 P12 CSE 2

E01 P01 ECE 4
E02 P02 ECE 4
E03 P03 ECE 3
E04 P04 ECE 4
E05 P05 ECE 3
E06 P06 ECE 4
E07 P07 ECE 3
E08 P08 ECE 3
E09 P13 ECE 4
E10 P14 ECE 3
E11 P15 ECE 3
E12 P09 ECE 2

M01 P10 ME 4
M02 P11 ME 4
M03 P12 ME 3
M04 P13 ME 4
M05 P14 ME 3
M06 P15 ME 4
M07 P01 ME 3
M08 P02 ME 3
M09 P03 ME 4
M10 P04 ME 3
M11 P05 ME 3
M12 P06 ME 2
*/