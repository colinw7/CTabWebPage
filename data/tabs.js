function openTab(evt, tabName, elmnt, color) {
  // Declare all variables
  var i, tabcontent, tabs;

  // Get all elements with class="tabcontent" and hide them
  tabcontent = document.getElementsByClassName("tab_content");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Get all elements with class="tab_button" and remove the class "tab_active"
  tabs = document.getElementsByClassName("tab_button");
  for (i = 0; i < tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" tab_active", "");
  }

  // Show the current tab, and add an "tab_active" class to the button that opened the tab
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " tab_active";
}

// Get the element with id="defaultOpen" and click on it
document.getElementById("defaultOpen").click(); 
