// =====================================================================
//
// Output function
//
// =====================================================================

/* ******************************************************************** */
void LCDML_lcd_menu_display()
/* ******************************************************************** */
{
  // for first test set font here
  u8g2.setFont(_LCDML_u8g_font); 
  //u8g2.setColorIndex(1); // Instructs the display to draw with a pixel on.

  if (LCDML_DISP_update()) {

    // init vars
    uint8_t n_max = (LCDML.getChilds() >= _LCDML_u8g_rows) ? ((_LCDML_u8g_rows > _LCDML_u8g_rows_max) ? _LCDML_u8g_rows : _LCDML_u8g_rows_max) : (LCDML.getChilds());

    // set page
    u8g2.firstPage();

    // generate content
    do {

      // display rows and cursor
      for (uint8_t n = 0; n < n_max; n++)
      {
        // set cursor
        if (n == LCDML.getCursorPos()) {
          u8g2.drawStr( 0, _LCDML_u8g_font_h * (n + 1),  "X");
        }
		
		// set content
		// with content id you can add special content to your static menu or replace the content
		// the content_id contains the id wich is set on main tab for a menuitem
        switch(LCDML.content_id[n])
		{		 
			//case 0:
			//	u8g.drawStr( 10, _LCDML_u8g_font_h * (n + 1), "var_datetime"); // or datetime or other things
			//	break;
		
			default: // static content
				u8g2.drawStr( 10, _LCDML_u8g_font_h * (n + 1), LCDML.content[n]);
				break;				
		}
      }

      // display scrollbar when more content as rows available 
      if (LCDML.getChilds() > n_max) {

        // set frame for scrollbar
        u8g2.drawFrame(_LCDML_u8g_lcd_w - _LCDML_u8g_scrollbar_w, 0, _LCDML_u8g_scrollbar_w, _LCDML_u8g_lcd_h);

        // calculate scrollbar length
        uint8_t scrollbar_block_length = LCDML.getChilds() - n_max;
        scrollbar_block_length = _LCDML_u8g_lcd_h / (scrollbar_block_length + _LCDML_DISP_rows);

        //set scrollbar
        if (LCDML.getCursorPosAbs() == 0) {                                   // top position     (min)
          u8g2.drawBox(_LCDML_u8g_lcd_w - (_LCDML_u8g_scrollbar_w-1), 1                                                    , (_LCDML_u8g_scrollbar_w-2)  , scrollbar_block_length);
        } 
        else if (LCDML.getCursorPosAbs() == (LCDML.getChilds())) {            // bottom position  (max)         
          u8g2.drawBox(_LCDML_u8g_lcd_w - (_LCDML_u8g_scrollbar_w-1), _LCDML_u8g_lcd_h - scrollbar_block_length            , (_LCDML_u8g_scrollbar_w-2)  , scrollbar_block_length);
        } 
        else {                                                                // between top and bottom
          u8g2.drawBox(_LCDML_u8g_lcd_w - (_LCDML_u8g_scrollbar_w-1), (scrollbar_block_length * LCDML.getCursorPosAbs() + 1),(_LCDML_u8g_scrollbar_w-2)  , scrollbar_block_length);
        }
      }
    } while ( u8g2.nextPage() );
  }

  // reinit some vars
  LCDML_DISP_update_end();
}

// lcd clear
void LCDML_lcd_menu_clear()
{
}