function openTab(evt, tabName, elmnt, color) {
  // Hide all elements with class="tabcontent" by default */
  var i, tabcontent, tabs;
  tabcontent = document.getElementsByClassName("tab_content");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Remove the background color of all tabs/buttons
  tabs = document.getElementsByClassName("tabs");
  for (i = 0; i < tabs.length; i++) {
    tabs[i].style.backgroundColor = "";
  }

  // Show the specific tab content
  document.getElementById(tabName).style.display = "block";

  // Add the specific color to the button used to open the tab content
  elmnt.style.backgroundColor = color;

  contents = document.getElementsByClassName("tab_content");
  for (i = 0; i < contents.length; i++) {
    contents[i].style.backgroundColor = color;
  }
}

// Get the element with id="defaultOpen" and click on it
document.getElementById("defaultOpen").click(); 
