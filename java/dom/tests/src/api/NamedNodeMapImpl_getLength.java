/*
 The contents of this file are subject to the Mozilla Public
 License Version 1.1 (the "License"); you may not use this file
 except in compliance with the License. You may obtain a copy of
 the License at http://www.mozilla.org/MPL/

 Software distributed under the License is distributed on an "AS
 IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 implied. See the License for the specific language governing
 rights and limitations under the License.

 The Original Code is mozilla.org code.

 The Initial Developer of the Original Code is Sun Microsystems,
 Inc. Portions created by Sun are
 Copyright (C) 1999 Sun Microsystems, Inc. All
 Rights Reserved.

 Contributor(s):
*/

package org.mozilla.dom.test;

import java.util.*;
import java.io.*;
import org.mozilla.dom.test.*;
import org.mozilla.dom.*;
import org.w3c.dom.*;

public class NamedNodeMapImpl_getLength extends BWBaseTest implements Execution
{

   /**
    *
    ***********************************************************
    *  Constructor
    ***********************************************************
    *
    */
   public NamedNodeMapImpl_getLength()
   {
   }


   /**
    *
    ***********************************************************
    *  Starting point of application
    *
    *  @param   args    Array of command line arguments
    *
    ***********************************************************
    */
   public static void main(String[] args)
   {
   }

   /**
    ***********************************************************
    *
    *  Execute Method 
    *
    *  @param   tobj    Object reference (Node/Document/...)
    *  @return          true or false  depending on whether test passed or failed.
    *
    ***********************************************************
    */
   public boolean execute(Object tobj)
   {
      if (tobj == null)  {
         TestLoader.logPrint("Object is NULL...");
         return BWBaseTest.FAILED;
      }

      String os = System.getProperty("OS");
      osRoutine(os);

      Document d = (Document)tobj;
      if (d != null)
      {
         String attrName = "TARGET";
         Attr a = d.createAttribute(attrName);
         if (a == null) {
             TestLoader.logErrPrint("Could not reate Attribute " + attrName); 
             return BWBaseTest.FAILED;
         }

         String aelement = "BASE";
         String attrValue = "";
         Element e = d.createElement(aelement);
         if (e != null)
         {
            a.setValue(attrValue);
            e.setAttributeNode(a);
         } else {
            TestLoader.logErrPrint("Element " + aelement + " could not be created..."); 
            return BWBaseTest.FAILED;
         }

         NamedNodeMap map = e.getAttributes();
         if (map != null) 
         {
             int len = map.getLength();
             System.out.println(" Map Length is  " + len);
         } else {
             TestLoader.logErrPrint(" Could not find Attriblist for node  " + aelement);
             return BWBaseTest.FAILED;
         } 
     } else {
         TestLoader.logErrPrint("Document is  NULL..");
         return BWBaseTest.FAILED;
     }

      return BWBaseTest.PASSED;
   }

   /**
    *
    ***********************************************************
    *  Routine where OS specific checks are made. 
    *
    *  @param   os      OS Name (SunOS/Linus/MacOS/...)
    ***********************************************************
    *
    */
   private void osRoutine(String os)
   {
     if(os == null) return;

     os = os.trim();
     if(os.compareTo("SunOS") == 0) {}
     else if(os.compareTo("Linux") == 0) {}
     else if(os.compareTo("Windows") == 0) {}
     else if(os.compareTo("MacOS") == 0) {}
     else {}
   }
}
