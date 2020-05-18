// visitor.cpp
#include <iostream>
#include <string>
#include <vector>
#include <map>

// using namespace std;

// 1.  declare Element types
class Element;
class This;
class That;
class TheOther;
typedef std::vector<Element*> Elements;
typedef Elements::iterator    Elements_it;

// 2. Create a "visitor" base class with a visit() method for every element type
class Visitor
{
public:
    virtual void visit(This *e) = 0;
    virtual void visit(That *e) = 0;
    virtual void visit(TheOther *e) = 0;
};

// 3. Add an accept(Visitor) method to the "element" hierarchy
class Element
{
public:
    virtual void accept(class Visitor &v) = 0;
};

class This: public Element
{
public:
    void accept(Visitor &v);
    std::string name() { return "This"; } ;
};

class That: public Element
{
public:
    void accept(Visitor &v);
    std::string name() { return "That"; };
};

class TheOther: public Element
{
public:
    void accept(Visitor &v);
    std::string name() { return "TheOther"; };
};

// 5. Define the Element type accept() method
void This::accept(Visitor &v)
{
    v.visit(this);
}

void That::accept(Visitor &v)
{
    v.visit(this);
}

void TheOther::accept(Visitor &v)
{
    v.visit(this);
}

// 6. Create a "visitor" derived class for each "operation" to do on "elements"
class FrenchVisitor: public Visitor
{
public:
    FrenchVisitor()
    {
    	dictionary["This"]     = "ce"      ;
    	dictionary["That"]     = "que"     ;
    	dictionary["TheOther"] = "l'autre" ;
    }
private:
    std::map<std::string,std::string> dictionary;

    void visit(This *e)
    {
        std::cout << "FrenchVisitor: " << dictionary[e->name()] << std::endl;
    }

    void visit(That *e)
    {
        std::cout << "FrenchVisitor: " <<  dictionary[e->name()] << std::endl;
    }
    void visit(TheOther *e)
    {
        std::cout << "FrenchVisitor: " << dictionary[e->name()] << std::endl;
    }
};

class UpperCaseVisitor: public Visitor
{
    std::string toUpper(std::string str) {
        // https://stackoverflow.com/questions/735204/convert-a-string-in-c-to-upper-case
        std::string result = str;
    	std::transform(result.begin(), result.end(),result.begin(), ::toupper);
    	return result;
    }
    void visit(This *e)
    {
        std::cout << "UpperCaseVisitor: " << toUpper(e->name()) << std::endl;
    }

    void visit(That *e)
    {
        std::cout << "UpperCaseVisitor: " <<  toUpper(e->name()) << std::endl;
    }

    void visit(TheOther *e)
    {
        std::cout << "UpperCaseVisitor: " << toUpper(e->name()) << std::endl;
    }
};


int main() {
    // create some elemenents
    Elements elements;
    elements.push_back(new This()    );
    elements.push_back(new That()    );
    elements.push_back(new TheOther());
    elements.push_back(new That()    );

    // traverse objects and visit them
    FrenchVisitor   frenchVisitor;
    for ( Elements_it it = elements.begin() ; it != elements.end() ; it++ ) {
        (*it)->accept(frenchVisitor);
    }

    UpperCaseVisitor UpperCaseVisitor;
    for ( Elements_it it = elements.begin() ; it != elements.end() ; it++ ) {
        (*it)->accept(UpperCaseVisitor);
    }

    return 0 ;
}
