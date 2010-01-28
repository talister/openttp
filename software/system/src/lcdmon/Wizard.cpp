//
//
//

#include "Sys.h"
#include "Debug.h"

#include "Button.h"
#include "Label.h"
#include "KeyEvent.h"
#include "Wizard.h"
#include "WidgetCallback.h"

Wizard::Wizard(Widget *parent):Dialog(parent)
{
	currentPage=0;
	Widget *lastPage = new Widget();
	lastPage->setGeometry(0,0,20,3);
	Label *l = new Label("Apply settings",lastPage);
	l->setGeometry(0,0,20,1);
	
	WidgetCallback<Wizard> *cb = new WidgetCallback<Wizard>(this,&Wizard::ok);
	Button *b = new Button("Yes",cb,lastPage);
	b->setGeometry(3,1,3,1);
	b->setFocusWidget(true);
	
	cb = new WidgetCallback<Wizard>(this,&Wizard::cancel);
	b = new Button("No",cb,lastPage);
	b->setGeometry(3,2,2,1);
	
	pages.push_back(lastPage);
	
	forcePaint=false; 
}

Wizard::~Wizard()
{
	Dout(dc::trace,"Wizard::~Wizard()");
	for (unsigned int i=0;i<pages.size();i++)
		delete pages[i];
	
	Dout(dc::trace,"Wizard::~Wizard() finished");
}

void Wizard::nextPage()
{
	Dout(dc::trace,"Wizard::nextPage() was " << currentPage);
	
	if (currentPage < pages.size())
	{
		currentPage++;
		forcePaint=true;
	}
	
	Dout(dc::trace,"Wizard::nextPage() now " << currentPage);
}

void Wizard::prevPage()
{
	Dout(dc::trace,"Wizard::prevPage() was " << currentPage);
	if (currentPage > 0)
	{
		currentPage--;
		forcePaint=true;
	}
	Dout(dc::trace,"Wizard::prevPage() now " << currentPage);
}

	
bool Wizard::keyEvent(KeyEvent &ke)
{
	
	if (pages[currentPage]->keyEvent(ke))
		return true;
	
	return false;
}

void Wizard::paint(std::vector<std::string> &display)
{
	std::string blank;
	blank.append(20,' ');
	display[0]=blank;
	display[1]=blank;
	display[2]=blank;
	display[3]=blank;
	
	pages[currentPage]->paint(display);
	
	//setDirty(false); - strictly, done in widgets
	forcePaint=false;
}

void  Wizard::focus(int *x,int *y)
{
	pages[currentPage]->focus(x,y);
}

bool Wizard::dirty()
{
	// this is a bodge - problem arises because Wizard has no children (and this is because we don't want to paint all pages,
	// pass all events etc ..
	bool ret=forcePaint;
	for (unsigned int i=0;i<pages.size();i++)
		ret = ret || pages.at(i)->dirty();
	return ret;
}

Widget * Wizard::addPage(std::string l)
{
	Widget *w = new Widget();
	
	if (pages.size()==1)
		pages.insert(pages.begin(),w);	// first page to front
	else
		pages.insert(--pages.end(),w); // others to before the last page
			
	Label *title = new Label(l,w);
	title->setGeometry(0,0,20,1);
	if (pages.size()==2)
	{
		WidgetCallback<Wizard> *cb = new WidgetCallback<Wizard>(this,&Wizard::nextPage);
		addCallback(cb);
		Button *button = new Button("Next>",cb,w);
		button->setGeometry(7,3,5,1);
	}
	else
	{
		WidgetCallback<Wizard> *cb = new WidgetCallback<Wizard>(this,&Wizard::prevPage);
		addCallback(cb);
		Button *button = new Button("<Prev",cb,w);
		button->setGeometry(2,3,5,1);
		
		cb = new WidgetCallback<Wizard>(this,&Wizard::nextPage);
		addCallback(cb);
		button = new Button("Next>",cb,w);
		button->setGeometry(11,3,5,1);
	}
	
	return w;
}
