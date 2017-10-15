/* ******************************************************************************                                                                        
 *                        LCDMenuLib (LCDML)                                                                                                       
 * ****************************************************************************** 
 *
 * MIT License
 * 
 * Copyright (c) [2017] [Nils Feldkämper]
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *                                      
 * ******************************************************************************
 *   
 * BUG / ISSUES REPORTING                                      
 *    https://github.com/Jomelo/LCDMenuLib/issues 
 * 
 * ARDUIONO FORUM                                                    
 *     http://forum.arduino.cc/index.php?topic=73816.0  
 *   
 * ****************************************************************************** 
 */

#include "LCDMenuLib.h"
#include "LCDMenuLib_typedef.h"

// ********************************************************************
// constructor
// @param
//         menu instance
//        LCD instance
//        flash table for menu elements
//        lcd rows
//        lcd colls
// @return
//        ---
// ********************************************************************
LCDMenuLib::LCDMenuLib(LCDMenuLib_menu &p_r, const uint8_t p_rows, const uint8_t p_cols, LCDML_FuncPtr contentUpdate, LCDML_FuncPtr contentClear, LCDML_FuncPtr   menuControl)
// ********************************************************************
{
    // initialisation   
    rootMenu        = &p_r;
    curMenu         = rootMenu;
    control         = 0;
    funcReg         = 0;
    cursor_pos      = 0;
    layer           = 0;
    layer_save[0]   = 0;
    child_cnt       = 0;
    rows            = p_rows;
    cols            = (p_cols-1); 
    activMenu       = NULL;
    button          = 0;
    
    menu_timer      = 0;
    menu_default_time = 100000000;
    

    BT_resetAll();    
    

    callback_contentUpdate          = contentUpdate;     // Update Content            
    callback_contentClear           = contentClear;      // Clear Content
    callback_menuControl            = menuControl;    // Trigger Backend
}



void LCDMenuLib::loop()
{
    // Check Control
    if(callback_menuControl != NULL)
    {
        callback_menuControl();
    }    
    
    //Screensaver
    if(cb_screensaver != NULL && bitRead(funcReg, _LCDML_funcReg_disable_screensaver) == false)
    {
        if(activMenu->callback_function != cb_screensaver) // check if screensaver is active
        {
            if(TIMER_ms(screensaver_timer, screensaver_default_time)) 
            {
                OTHER_jumpToFunc(cb_screensaver);
            }
        }
    }
    
    // Check Function
    if(activMenu != NULL)
    {        
        if(TIMER_ms(menu_timer, menu_default_time) || bitRead(funcReg, _LCDML_funcReg_setup) == 0 || button > 0) 
        {            
            FUNC_call();            
        }
    } else {        
        menu_default_time = 100000000;
    }
}



/* ******************************************************************** *
 * private
 * @param
 *        menu instance (pointer)
 *        menu element (pointer)
 * @return
 *        search status (uint8)
 * ******************************************************************** */
boolean        LCDMenuLib::MENU_selectElementDirect(LCDMenuLib_menu &p_m, uint8_t p_search)
/* ******************************************************************** */
{
    //deklaration
    LCDMenuLib_menu *search = &p_m;
    LCDMenuLib_menu *tmp;    
    boolean found    = 0;   
    
    do 
    {
        if (search->getChild(0) != NULL) 
        {
            tmp = search->getChild(0);
            
            if(tmp->checkCallback() && tmp->getParam() <= 200) 
            {
                if (tmp->getID() == p_search) 
                { //search elements in this layer            
                    found = true;                
                    break;
                } 

                found = MENU_selectElementDirect(*tmp, p_search); //recursive search until found is true or last item reached
                            
                if (found == true) //something found
                {                     
                    break; 
                } 
                else 
                {  
                    // go down
                    if (curloc < child_cnt) 
                    { 
                        curloc++; 
                        MENU_doScroll();
                    }                     
                    child_cnt = MENU_countChilds();
                }                
            } 
            else
            {   
                //check elements for childs                    
                MENU_goInto();
                
                if (tmp->getID() == p_search) 
                { 
                    //search elements in this layer            
                    found = true;                
                    break;
                }               

                found = MENU_selectElementDirect(*tmp, p_search); //recursive search until found is true or last item reached
                            
                if (found == true) //something found
                {   
                    break; 
                } 
                else 
                {  
                    //quit layer, go to next element
                    MENU_goBack();                    
                    child_cnt = MENU_countChilds();
                    
                    if (curloc < child_cnt) 
                    { 
                        curloc++; 
                        MENU_doScroll();
                    } 
                    //child_cnt = MENU_countChilds();
                }  
            }           
        } 
        else 
        {            
            //no childs found                
            if (search->getID() == p_search) //found something
            {  
                found = true;                
                break;
            } 
            else 
            {                
                //select next element            
                if (curloc < child_cnt) { 
                    curloc++; 
                    MENU_doScroll();
                }                
            }
        }        
    } while ((search=search->getSibling(1)) != NULL && found == 0);
       
    
    return found;     
}


/* ******************************************************************** *
 * private
 * @param
 *        menu instance (pointer)
 *        menu element (pointer)
 * @return
 *        search status (uint8)
 * ******************************************************************** */
boolean     LCDMenuLib::MENU_selectElementDirect2(LCDMenuLib_menu &p_m, LCDML_FuncPtr2 p_search)
/* ******************************************************************** */
{
    //deklaration
    LCDMenuLib_menu *search = &p_m;
    LCDMenuLib_menu *tmp;    
    boolean found    = 0;   
        
    do 
    {  
        if (search->getChild(0) != NULL) 
        {
            tmp = search->getChild(0);
            
            if(tmp->checkCallback() && tmp->getParam() <= 200) 
            {
                if (tmp->callback_function == p_search) 
                { //search elements in this layer            
                    found = true;                
                    break;
                } 

                found = MENU_selectElementDirect2(*tmp, p_search); //recursive search until found is true or last item reached
                            
                if (found == true) //something found
                {                     
                    break; 
                } 
                else 
                {  
                    // go down
                    if (curloc < child_cnt) 
                    { 
                        curloc++; 
                        MENU_doScroll();
                    } 
                    child_cnt = MENU_countChilds();     
                }                
            } 
            else
            {   
                //check elements for childs                    
                MENU_goInto();               
                
                if (tmp->callback_function == p_search) 
                { 
                    //search elements in this layer            
                    found = true;                
                    break;
                }               

                found = MENU_selectElementDirect2(*tmp, p_search); //recursive search until found is true or last item reached
                            
                if (found == true) //something found
                {   
                    break; 
                } 
                else 
                {  
                    //quit layer, go to next element
                    MENU_goBack();
                    child_cnt = MENU_countChilds();
                    
                    if (curloc < child_cnt) 
                    { 
                        curloc++; 
                        MENU_doScroll();
                    }
                }  
            }           
        } 
        else 
        {            
            //no childs found                
            if (search->callback_function == p_search) //found something
            {  
                found = true;                
                break;
            } 
            else 
            {                
                //select next element            
                if (curloc < child_cnt) { 
                    curloc++; 
                    MENU_doScroll();
                }                
            }
        }        
    } while ((search=search->getSibling(1)) != NULL && found == 0);    
      
    //return result
    return found;    
}


/* ******************************************************************** *
 * public: go to the first element in this menu with id = 0
 * @param
 * @return
 * ******************************************************************** */
void        LCDMenuLib::MENU_goRoot()
/* ******************************************************************** */
{
    if(activMenu != NULL) 
    {               
        bitSet(funcReg, _LCDML_funcReg_end);
        FUNC_call();  // call active function for save close;
    } 
    
    BT_resetAll();
    bitClear(funcReg, _LCDML_funcReg_disable_screensaver);
    bitClear(funcReg, _LCDML_funcReg_end);
    bitClear(funcReg, _LCDML_funcReg_setup);        
    
    
    curMenu = rootMenu;
    
    layer = 0;
    cursor_pos = 0;
    curloc = 0;
    scroll = 0;        

    button = 0;    
    
    MENU_display();
    
    if(bitRead(control, _LCDML_control_search_display) == false)
    {
        if(activMenu != NULL)
        {
            activMenu = NULL;
            DISP_menuUpdate();
        }
    } 
}

/* ******************************************************************** *
 * private: curloc correction
 * @param
 * @return
 *    correction (uint8)
 * ******************************************************************** */
uint8_t        LCDMenuLib::MENU_curlocCorrection()
{
    uint8_t    curloc_cor = 0;
    uint8_t j = 0;
    LCDMenuLib_menu *tmp;
    
    //correct function / menu element to open for hidden menu elements
    if ((tmp = curMenu->getChild(0)) != NULL) 
    {
        do
        {
            if (tmp->checkCondetion() || bitRead(control, _LCDML_control_disable_hidden)) 
            {
                j++;
            }
            else 
            {
                if (j <= curloc) 
                {
                    curloc_cor++;
                }
            }
        } while ((tmp = tmp->getSibling(1)) != NULL);
    }
    return curloc_cor;
}


/* ******************************************************************** *
 * public: go to the first element in this menu with id = 0
 * @param
 * @return
 * ******************************************************************** */
void    LCDMenuLib::MENU_goBack(void)
/* ******************************************************************** */
{    
    if(curMenu->getParent() != NULL)
    {
        //go back
        bitWrite(control, _LCDML_control_menu_back, 1);
        //set current menu object         
        MENU_goMenu(*curMenu->getParent());       
    } 
    else
    {
        statusCode = 1;
    }
}

/* ******************************************************************** *
 * public: go into a menu 
 * @param
 * @return
 * ******************************************************************** */
void    LCDMenuLib::MENU_goInto(void)
/* ******************************************************************** */
{    
    LCDMenuLib_menu *tmp;
    
    // check if a menu function is not active
    if (activMenu == NULL)
    {
        // get element to to open
        tmp = curMenu->getChild(curloc + MENU_curlocCorrection());
                        
        // check if element is a menu function
        if(tmp->checkCallback() && tmp->getParam() <= 200)
        {
            // Menufunction found
            activMenu = tmp;
        }
        else
        { 
            if(tmp->getParam() == 201)
            {
                MENU_display();
                DISP_menuUpdate();                                    
            } 
            else 
            {
                if(tmp->getChild(0) != NULL)
                {
                    while ((tmp = tmp->getSibling(1)) != NULL)
                    {                   
                        if (tmp->checkCondetion() || bitRead(control, _LCDML_control_disable_hidden)) 
                        {                   
                            // Menu found, goInto
                            MENU_goMenu(*curMenu->getChild(curloc + MENU_curlocCorrection()));
                            
                            if(bitRead(control, _LCDML_control_search_display) == false)
                            {
                                BT_resetAll(); // reset all buttons
                                
                                child_cnt = MENU_countChilds();
                                
                                MENU_display();
                                DISP_menuUpdate();
                            }
                            break;
                        }                    
                    }  
                }  
            }                
        }
    }
}

/* ******************************************************************** *
 * private: go in a menu item and display next elements
 * @param
 *        pointer to next menu item
 * @return
 * ******************************************************************** */
void    LCDMenuLib::MENU_goMenu(LCDMenuLib_menu &m)
/* ******************************************************************** */
{
    //declare variables
    int diff;

    curMenu = &m;
            
    if(layer < _LCDML_DISP_cfg_cursor_deep) { //check layer deep        
        if(bitRead(control, _LCDML_control_menu_back) == 0) { //check back button        
            // go into the next layer
            // save old layer data
            layer_save[layer] = curloc;
            scroll_save[layer] = scroll;
            // new layer
            scroll = 0;
            layer++;            
            // save parent id
            if(layer < _LCDML_DISP_cfg_cursor_deep) {
                parents[layer] = curMenu->getID();
            }            
            curloc = 0;
        } 
        else {
            //button reset
            bitWrite(control, _LCDML_control_menu_back, 0);
            
            if(layer > 0) {                
                layer--; 
                scroll = scroll_save[layer];
                curloc = layer_save[layer];

                if(curloc >= rows) {
                    diff = curloc-(rows-1);
                    for(int i=0; i<diff; i++) 
                    {                        
                        MENU_doScroll();
                    }
                }
            }
        }
    }
}


/* ******************************************************************** *
 * private: count sibling for an menu item if exists
 * @param
 * @return
 *    sibling count 0 = one 
 * ******************************************************************** */
uint8_t    LCDMenuLib::MENU_countChilds()
/* ******************************************************************** */
{
    //declaration
    uint8_t j = 0;
    LCDMenuLib_menu *tmp;
    
    //check if element has childs
    if ((tmp = curMenu->getChild(0)) != NULL) {    
        do
        {                
            if (tmp->checkCondetion() || bitRead(control, _LCDML_control_disable_hidden)) {                
                j++;
            } 
        } while ((tmp = tmp->getSibling(1)) != NULL);        
    }    

    if(j == 0) {
        return 0;
    } else {    
        return --j;
    }
}


LCDMenuLib_menu * LCDMenuLib::MENU_getObj()
{
    return curMenu;
}

/* ******************************************************************** *
 * public: display the current menu
 * @param
 * @return
 * ******************************************************************** */
void    LCDMenuLib::MENU_display(uint8_t update)
/* ******************************************************************** */
{
    //declaration
    uint8_t i = scroll;
    uint8_t maxi = (rows + scroll);
    LCDMenuLib_menu *tmp;
        
    //check children
    if ((tmp = curMenu->getChild(i))) 
    { 
        bitSet(control, _LCDML_control_disp_update);
    }    
    MENU_setCursor();
}



uint8_t LCDMenuLib::MENU_getScroll()
{
    return scroll;
}


/* ******************************************************************** *
 * private: set cursor position
 * @param
 * @return
 * ******************************************************************** */
void    LCDMenuLib::MENU_setCursor()
/* ******************************************************************** */
{
    child_cnt = MENU_countChilds();    

    //save current cursor position
    cursor_pos = curloc - scroll;
    
    if (cursor_pos > child_cnt) {
        cursor_pos = child_cnt;
    }    
    
    bitSet(control, _LCDML_control_cursor_update);
}


/* ******************************************************************** *
 * private: do scroll if lines in display to small to show all items
 * @param
 * @return
 * ******************************************************************** */
void    LCDMenuLib::MENU_doScroll()
/* ******************************************************************** */
{    
    //only allow it to go up to menu element   
    while (curloc>0 && !curMenu->getChild(curloc))
    {
        curloc--;
    }
    //scroll down
    if (curloc >= (rows + scroll))
    {
        scroll++;
        if(bitRead(control, _LCDML_control_search_display) == false) 
        {
            MENU_display();
        }
    }
    //scroll up
    else if (curloc < scroll)
    {
        scroll--;
        if(bitRead(control, _LCDML_control_search_display) == false) 
        {
            MENU_display();
        }
    }
    //do not scroll
    else
    {
        MENU_setCursor();
    }
} 



void    LCDMenuLib::MENU_enRollover(void)
{
    bitSet(control, _LCDML_control_rollover);
}

void    LCDMenuLib::MENU_disRollover(void)
{
    bitClear(control, _LCDML_control_rollover);
}









/* ******************************************************************** *
 * public: get the name of the active function
 * @param
 * @return
 *    active function (uint8)
 * ******************************************************************** */
uint8_t    LCDMenuLib::FUNC_getID()
/* ******************************************************************** */
{ 
    if(activMenu != NULL) {
        return activMenu->getID();
    } else {
        return _LCDML_NO_FUNC;
    }        
}


void    LCDMenuLib::FUNC_setLoopInterval(unsigned long t)
{
    menu_default_time = t;
}





/* ******************************************************************** *
 * public: return the position of the current layer
 * @param
 * @return
 *    layer (uint8)
 * ******************************************************************** */
uint8_t    LCDMenuLib::MENU_getLayer()
/* ******************************************************************** */
{
    return layer;
}


/* ******************************************************************** *
 * public: the current cursor position with correcture for hidden elements
 * @param
 * @return
 *    current corsor position
 * ******************************************************************** */
uint8_t    LCDMenuLib::MENU_getCursorPos()
/* ******************************************************************** */
{    
    return (cursor_pos); //return the current cursor position
}

/* ******************************************************************** *
 * public: the current cursor position with correcture for hidden elements
 * @param
 * @return
 *    current corsor position
 * ******************************************************************** */
uint8_t    LCDMenuLib::MENU_getCursorPosAbs()
/* ******************************************************************** */
{    
    return (curloc + MENU_curlocCorrection()); //return the current cursor position
}


/* ******************************************************************** *
 * public: return the cnt of childs on next layer
 * @param
 * @return
 *    child cnt
 * ******************************************************************** */
uint8_t LCDMenuLib::MENU_getChilds()
/* ******************************************************************** */
{
    return child_cnt+1;
}


/* ******************************************************************** *
 * public: get parent id 
 * @param
 * @return
 *    return the current parent id when the layer is > 0
 * ******************************************************************** */
uint8_t LCDMenuLib::MENU_getParentId()
/* ******************************************************************** */
{
    if(layer > 0 && layer < _LCDML_DISP_cfg_cursor_deep) {
        return parents[layer];
    } else {
        return _LCDML_NO_FUNC;
    }    
}

/* ******************************************************************** *
 * public: get parent id from a special layer
 * @param
 * @return
 *    return the current parent id when the layer is > 0
 * ******************************************************************** */
uint8_t LCDMenuLib::MENU_getParentId(uint8_t p_layer)
/* ******************************************************************** */
{
    if(p_layer > 0 && p_layer < _LCDML_DISP_cfg_cursor_deep) {
        return parents[p_layer];
    } else {
        return _LCDML_NO_FUNC;
    }    
}


/* ******************************* 
   DISP_
 * ******************************* */

 

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::DISP_menuUpdate()
{    
    if(activMenu == NULL || bitRead(funcReg, _LCDML_funcReg_end) || bitRead(control, _LCDML_control_update_direct)) 
    { 
        callback_contentUpdate();
    }
    bitClear(control, _LCDML_control_disp_update);
    bitClear(control, _LCDML_control_cursor_update);
    bitClear(control, _LCDML_control_update_direct); 
            
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::DISP_clear()
{
    callback_contentClear();    
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::DISP_checkMenuUpdate()
{
    if (bitRead(control, _LCDML_control_disp_update) || bitRead(control, _LCDML_control_update_direct)) { 
        return true; 
    } else { 
        return false; 
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::DISP_checkMenuCursorUpdate()
{
    if(bitRead(control, _LCDML_control_cursor_update)) { 
        return true; 
    } else { 
        return false;
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::FUNC_call()
{
    if(activMenu != NULL) 
    {
        activMenu->callback(activMenu->getParam());
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::FUNC_setup()
{    
    if(activMenu != NULL) 
    {
        // function active 
        if(bitRead(funcReg, _LCDML_funcReg_setup) == 0) {           
            bitSet(funcReg, _LCDML_funcReg_setup); // run setup
            DISP_clear();
            BT_resetAll();
            return true;
        } 
        else
        {
            return false;
        }         
    } 
    else 
    {
        bitClear(funcReg, _LCDML_funcReg_setup);
        return false; 
    }      
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::FUNC_loop()
{
    if(activMenu != NULL) 
    {
        if(bitRead(funcReg, _LCDML_funcReg_end) == false) {
            return true;
        } else {
            return false;
        }  
    }
    else
    {
        return false;
    }    
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::FUNC_stableEnd()
{
    if(activMenu != NULL) 
    {
        if(bitRead(funcReg, _LCDML_funcReg_end) == true) 
        {
            bitClear(funcReg, _LCDML_funcReg_setup);
            activMenu = NULL;
            return true;
        } else {
            return false;
        }
    }
    else
    {
        return false;
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::FUNC_goBackToMenu(uint8_t e)
{
    if(activMenu != NULL) 
    {       
        bitSet(funcReg, _LCDML_funcReg_end);        
        FUNC_call();  // call active function for save close;
        activMenu = NULL;        
    }
    BT_resetAll();
    bitClear(funcReg, _LCDML_funcReg_disable_screensaver);
    bitClear(funcReg, _LCDML_funcReg_end);
    bitClear(funcReg, _LCDML_funcReg_setup);       
   
    if(e == 1) {
        MENU_goBack();
    }
    MENU_display();
    DISP_menuUpdate();    
}



void    LCDMenuLib::FUNC_disableScreensaver()
{
    bitSet(funcReg, _LCDML_funcReg_disable_screensaver);
}




/* ******************************* 
   BT_
 * ******************************* */


/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */ 
boolean LCDMenuLib::BT_setup()
{
    if(bitRead(control, _LCDML_control_setup) == 0) 
    {
        bitSet(control, _LCDML_control_setup);
        return true;
    }        
    else
    {
        return false;
    }    
    
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::BT_enter()             
{ 
    bitSet(button, _LCDML_button_enter);
    SCREEN_resetTimer();
    
    if(activMenu == NULL) 
    {            
        //menu is active      
        MENU_goInto();        
        bitSet(control, _LCDML_control_update_direct);
    }
    
} 

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::BT_up()                
{ 
    bitSet(button, _LCDML_button_up);
    SCREEN_resetTimer();

    if(activMenu == NULL) 
    {
        if (curloc > 0) { 
            curloc--; 
            MENU_doScroll();             
        } 
        else
        {
            if(bitRead(control, _LCDML_control_rollover))
            {
                // jump to the end of the menu
                curloc = child_cnt;
                if(child_cnt-rows < 0) {
                    scroll = 0;
                } else {
                    scroll = child_cnt-rows;
                }                    
                MENU_doScroll();
                MENU_display();
            }            
        }
        
        DISP_menuUpdate(); 
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::BT_down()              
{
    bitSet(button, _LCDML_button_down);
    SCREEN_resetTimer();
    
    if(activMenu == NULL) 
    {
        if(curloc < child_cnt) { 
            curloc++; 
            MENU_doScroll();
        }
        else
        {
            if(bitRead(control, _LCDML_control_rollover))
            {
                // jump to the first line
                curloc = 0;
                scroll = 0;                
                MENU_doScroll();
                MENU_display();                
            }
        }        
        DISP_menuUpdate(); 
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::BT_left()              
{
    bitSet(button, _LCDML_button_left);
    SCREEN_resetTimer();
    
    if(activMenu == NULL)
    {
        MENU_display();
        DISP_menuUpdate();
    }    
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::BT_right()             
{ 
    bitSet(button, _LCDML_button_right);
    SCREEN_resetTimer();
    
    if(activMenu == NULL)
    {
        MENU_display();
        DISP_menuUpdate();
    }        
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void LCDMenuLib::BT_quit()              
{ 
    SCREEN_resetTimer();
    
    if(activMenu != NULL) 
    {
        FUNC_goBackToMenu();
    } 
    else 
    {
        if(layer > 0) 
        {
            MENU_goBack();
            MENU_display();
            DISP_menuUpdate();
        }
    } 
    
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */    
boolean LCDMenuLib::BT_checkAny()       { if((button > 0)) { return true; } else { return false; }                              }     

boolean LCDMenuLib::BT_checkEnter()     { if(bitRead(button, _LCDML_button_enter)) { return true; } else { return false; }      }
boolean LCDMenuLib::BT_checkUp()        { if(bitRead(button, _LCDML_button_up)) { return true; } else { return false; }         }     
boolean LCDMenuLib::BT_checkDown()      { if(bitRead(button, _LCDML_button_down)) { return true; } else { return false; }       }  
boolean LCDMenuLib::BT_checkLeft()      { if(bitRead(button, _LCDML_button_left)) { return true; } else { return false; }       }  
boolean LCDMenuLib::BT_checkRight()     { if(bitRead(button, _LCDML_button_right)) { return true; } else { return false; }      }  
 
    
void LCDMenuLib::BT_resetAll()          { button = 0;}

void LCDMenuLib::BT_resetEnter()        { bitClear(button, _LCDML_button_enter);    }
void LCDMenuLib::BT_resetUp()           { bitClear(button, _LCDML_button_up);       }
void LCDMenuLib::BT_resetDown()         { bitClear(button, _LCDML_button_down);     }
void LCDMenuLib::BT_resetLeft()         { bitClear(button, _LCDML_button_left);     }
void LCDMenuLib::BT_resetRight()        { bitClear(button, _LCDML_button_right);    } 



/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void    LCDMenuLib::TIMER_msReset(unsigned long &var)
{
    var = millis();
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::TIMER_ms(unsigned long &var, unsigned long t)
{
    if((millis() - var) >= t) 
    {
        var = millis(); 
        return true;
    }
    else
    {
        return false;
    }
}


/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
void    LCDMenuLib::TIMER_usReset(unsigned long &var)
{
    var = micros();
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::TIMER_us(unsigned long &var, unsigned long t)
{
    if((micros() - var) >= t) 
    {
        var = micros(); 
        return true;
    }
    else
    {
        return false;
    }
}

/* ******************************************************************** *
 * public: 
 * @param
 * @return
 *    
 * ******************************************************************** */
boolean LCDMenuLib::OTHER_jumpToFunc(uint8_t id)
{
    bitSet(control, _LCDML_control_disable_hidden);
    bitSet(control, _LCDML_control_search_display);
    // got to root
    MENU_goRoot();
    
    if(MENU_selectElementDirect(*rootMenu, id)) 
    { 
        bitClear(control, _LCDML_control_search_display);
        MENU_goInto();
        bitClear(control, _LCDML_control_disable_hidden);        
        return true;
    } 
    else
    {
        bitClear(control, _LCDML_control_search_display);
        return false;
    }
}


boolean LCDMenuLib::OTHER_jumpToFunc(LCDML_FuncPtr2 p_search)
{
    bitSet(control, _LCDML_control_disable_hidden);  
    bitSet(control, _LCDML_control_search_display);
    // got to root
    MENU_goRoot();
    
    if(MENU_selectElementDirect2(*rootMenu, p_search)) 
    {  
        bitClear(control, _LCDML_control_search_display);
        MENU_goInto();
        bitClear(control, _LCDML_control_disable_hidden);        
        return true;
    } 
    else
    {
        bitClear(control, _LCDML_control_search_display);
        return false;
    }
}



void    LCDMenuLib::SCREEN_enable(LCDML_FuncPtr2 function, unsigned long t)
{
    cb_screensaver = function;
    screensaver_default_time = t;
    
}


void    LCDMenuLib::SCREEN_disable()
{
   cb_screensaver = NULL;
}


void    LCDMenuLib::SCREEN_resetTimer()
{
    TIMER_msReset(screensaver_timer);    
}


boolean LCDMenuLib::DP_check(uint8_t n)
{   
    LCDMenuLib_menu *tmp;
    
    // check if a menu function is not active          
    tmp = curMenu->getChild(curloc + MENU_curlocCorrection());
      
    if(tmp != NULL)
    {
        if(tmp->getParam() > 200 && tmp->checkCallback())
        {
            tmp->callback(n);
            return true;
        }
    }
    
    // call dp callback
    return false;
}


















