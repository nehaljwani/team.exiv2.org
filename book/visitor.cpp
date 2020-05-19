// visitor.cpp
#include <iostream>
#include <string>
#include <vector>
#include <map>

// 1.  declare types
class   Student; // forward

// 2. Create abstract "visitor" base class with an element visit() method
class Visitor
{
public:
    Visitor() {};
    virtual void visit(Student& student) = 0 ;
};

// 3. Student has an accept(Visitor&) method
class Student
{
public:
    Student(std::string name,int age) : name_(name), age_(age) {}
    void accept(class Visitor& v) { v.visit(*this); }
    std::string name()  { return name_; } 
    int         age()   { return age_;  }
private:
    std::string name_ ;
    int         age_  ;
};

// 4. Create concrete "visitors"
class FrenchVisitor: public Visitor
{
public:
    FrenchVisitor()
    {
    	dictionary_["this"]      = "ce"      ;
    	dictionary_["that"]      = "que"     ;
    	dictionary_["the other"] = "l'autre" ;
    }
    void visit(Student& student)
    {
        std::cout << "FrenchVisitor: " << dictionary_[student.name()] << std::endl;
    }
private:
    std::map<std::string,std::string> dictionary_;
};

class AverageAgeVisitor: public Visitor
{
public:
    AverageAgeVisitor() : students_(0), years_(0) {}
    void visit(Student& student)
    {
        students_ ++ ;
        years_    += student.age();
    }
    void reportAverageAge() 
    {
        std::cout << "average age = "  << (double) years_ / (double) students_ << std::endl ;
    }
private:
    int years_;
    int students_;
};

int main() {
    // create students
    std::vector<Student>   students;
    students.push_back(Student("this",10)     );
    students.push_back(Student("that",12)     );
    students.push_back(Student("the other",14));

    // traverse objects and visit them
    FrenchVisitor    frenchVisitor;
    for ( std::vector<Student>::iterator student = students.begin() ; student != students.end() ; student++ ) {
        student->accept(frenchVisitor);
    }

    AverageAgeVisitor averageAgeVisitor;
    for ( std::vector<Student>::iterator student = students.begin() ; student != students.end() ; student++ ) {
        student->accept(averageAgeVisitor);
    }
    averageAgeVisitor.reportAverageAge();

    return 0 ;
}
