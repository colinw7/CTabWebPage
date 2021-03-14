initSlides();

function initSlides() {
  var slides = document.getElementsByClassName("image_container");
  var n = slides.length
  slideIndex = new Array(n)
  var i;
  for (i = 0; i < n; i++) {
    slideIndex[i] = 1;

    showSlides(i + 1, slideIndex[i]);
  }
}

// Next/previous controls
function plusSlides(id, n) {
  showSlides(id, slideIndex[id - 1] += n);
}

// Thumbnail image controls
function currentSlide(id, n) {
  showSlides(id, slideIndex[id - 1] = n);
}

function showSlides(id, n) {
  var slides = document.getElementsByClassName("image_slides_" + id);
  var dots   = document.getElementsByClassName("image_dot_"    + id);

  if (n > slides.length) {
    slideIndex[id - 1] = 1
  }

  if (n < 1) {
    slideIndex[id - 1] = slides.length
  }

  var i;

  for (i = 0; i < slides.length; i++) {
    slides[i].style.display = "none";
  }

  for (i = 0; i < dots.length; i++) {
    dots[i].className = dots[i].className.replace(" image_active", "");
  }

  slides[slideIndex[id - 1] - 1].style.display = "block";

  dots[slideIndex[id - 1] - 1].className += " image_active";
}
