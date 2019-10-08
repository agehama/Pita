#include <Pita/TreeLogger.hpp>
#include <Pita/Util.hpp>

namespace cgl
{
	void TreeLogger::printHead()
	{
		const std::string defaultStyle(R"(
html {
  height: 100%;
}
body {
  margin: 0;
  height: 100%;
}
.ps__rail-y {
  opacity: 1.0!important;
  width: 15px!important;
  background-size:100% 100% !important;
}
.ps__thumb-y {
  width: 10px!important;
}
details {
  background: #FFF;
  margin: 7px 0;
  padding: 0 0 1px 0;
  border: solid #00a8ff;
  border-width: 0px 0px 1px 2px;
}
summary {
  background: #00a8ff;
  color: #FFF;
  font-weight: bold;
  padding: 3px;
  margin: 0 0px 5px -2px;
}
.bottomButton {
  position: fixed;
  z-index: 99;
  font-size: 18px;
  border: none;
  outline: none;
  background-color: #333;
  color: #f2f2f2;
  cursor: pointer;
  padding: 15px;
}
.bottomButton:hover {
  background: #ddd;
  color: black;
}
#expandButton {
  bottom: 20px;
  right: 30px;
}
#closeButton {
  bottom: 72px;
  right: 30px;
})");

		if (ofs.is_open())
		{
			ofs << "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"UTF-8\" />" << std::endl;
			ofs << std::string("<style>\n") + defaultStyle + "</style>" << std::endl;
			ofs << "</head>\n<body>" << std::endl;
			ofs << R"*(<button onclick="openAll()" class="bottomButton" id="expandButton">Expand all</button>)*" << std::endl;
			ofs << R"*(<button onclick="closeAll()" class="bottomButton" id="closeButton">Close all</button>)*" << std::endl;
			ofs << R"(<div class="article">)" << std::endl;
		}
	}

	void TreeLogger::printTail()
	{
		const std::string inlineScript(R"*(
const offscreen = document.createElement('canvas');
offscreen.width = 10;
offscreen.height = 1000;
const offscreenContext = offscreen.getContext('2d');
offscreenContext.fillRect(0, 0, 1000, 250);
var container = document.querySelector('.article');
var markerHeight = '1%';
var staticStyle = document.createElement('style');
container.appendChild(staticStyle);
var dynamicStyle = document.createElement('style');
container.appendChild(dynamicStyle);
var scrollbarElement = document.getElementById('output-area');
var ps;
//webHydLa
if(scrollbarElement)
{
  ps = new PerfectScrollbar('#output-area',{minScrollbarLength: 20});
}
//native HyLaGI output
else
{
  staticSheet = staticStyle.sheet;
  staticSheet.insertRule('.article {order: 1; flex: 1; width: 100%; height: 100%; position: relative; overflow-y: scroll; overflow-x: visible; display: block;}', 0);
  ps = new PerfectScrollbar('.article',{minScrollbarLength: 20});
}
var dynamicSheet = dynamicStyle.sheet;
const colors = ["rgb(255,255,255)","rgb(248,231,76)","rgb(248,191,40)","rgb(240,136,35)","rgb(231,89,30)","rgb(209,38,98)","rgb(173,0,142)","rgb(120,0,146)","rgb(43,0,118)","rgb(0,0,0)"];
var currentRuleIndex = null;
function updateScrollMarker(markers)
{
  offscreenContext.fillStyle = "rgb(255,255,255)";
  offscreenContext.fillRect(0, 0, offscreen.width, offscreen.height);
  for(var i = 0; i < markers.length; i++)
  {
    currentMarker = markers[i];
    
    offscreenContext.fillStyle = colors[Math.floor(currentMarker.time*colors.length)];
    offscreenContext.fillRect(0, currentMarker.pos*offscreen.height-2, offscreen.width, 5);
  }
  if(currentRuleIndex != null)
  {
    dynamicSheet.deleteRule(currentRuleIndex);
  }
  styleText = '.ps__rail-y { background: url(\"' + offscreen.toDataURL() + '\"); }';
  currentRuleIndex = dynamicSheet.insertRule(styleText, 0);
}
timers = document.getElementsByClassName('timer');
var scrollOffset;
var scrollHeight;
var innerAutoProcess = false;
function reloadMakers() {
  if(innerAutoProcess)
  {
    return;
  }
  if(scrollbarElement)
  {
    scrollOffset = scrollbarElement.scrollTop;
    scrollHeight = scrollbarElement.scrollHeight;
  }
  else
  {
    scrollOffset = container.scrollTop;
    scrollHeight = container.scrollHeight;
  }
  ps.update();
  threshold = 1.0;
  var markerDictionary = {}
  function getElementPos(element)
  {
    var posProgress = (element.getBoundingClientRect().top + scrollOffset) / scrollHeight;
    return (posProgress).toFixed(3);
  }
  for(var i = 0; i < timers.length; i++)
  {
    currentTime = Number(timers[i].innerText);
    if(threshold < currentTime)
    {
      threshold = currentTime;
    }
    var timerPosElem = $(timers[i]);
    var currentDetails = $(timers[i]).closest('details');
    var varOutermostClosedDetails = null;
    while(true)
    {
      if(!currentDetails.attr('open'))
      {
        varOutermostClosedDetails = currentDetails;
      }
      parent = currentDetails.parent().closest('details');
      if(parent.length == 0)
      {
        break;
      }
      currentDetails = parent;
    }
    var posElement = varOutermostClosedDetails ? varOutermostClosedDetails.get(0) : timerPosElem.get(0);
    key = getElementPos(posElement);
    if(key in markerDictionary)
    {
      markerDictionary[key] += currentTime;
    }
    else
    {
      markerDictionary[key] = currentTime;
    }
  }
  console.log("threshold: ", threshold);
  markers = [];
  for (var key in markerDictionary)
  {
    //[0, 1)
    colorRate = Math.min(markerDictionary[key], threshold - 1) / threshold;
    markers.push({pos:key, time:colorRate});
  }
  markers.sort(function(a, b){
    return a.time - b.time;
  });
  updateScrollMarker(markers);
}
function openAll() {
  var x = document.getElementsByTagName("details");
  for (var i = 0; i < x.length; i++) {
    x[i].setAttribute("open", "true");
  }
  innerAutoProcess = false;
  reloadMakers();
  innerAutoProcess = true;
  setTimeout(function(){ innerAutoProcess = false; }, 1000);
}
function closeAll() {
  var x = document.getElementsByTagName("details");
  for (var i = 0; i < x.length; i++) {
    x[i].removeAttribute("open");
  }
  innerAutoProcess = false;
  reloadMakers();
  innerAutoProcess = true;
  setTimeout(function(){ innerAutoProcess = false; }, 1000);
})*");

		if (ofs.is_open())
		{
			ofs << "</div>" << std::endl;
			ofs << R"(<script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/jquery/1.9.1/jquery.min.js"></script>)" << std::endl;
			ofs << R"(<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/jquery.perfect-scrollbar/1.4.0/css/perfect-scrollbar.min.css"/>)" << std::endl;
			ofs << R"(<script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/jquery.perfect-scrollbar/1.4.0/perfect-scrollbar.min.js"></script>)" << std::endl;
			ofs << R"(<script type="text/javascript">)" << std::endl;
			ofs << inlineScript << std::endl;
			ofs << "</script>" << std::endl;

			ofs << "</body>\n</html>" << std::endl;
		}
	}

	std::string TreeLogger::escaped(const std::string& str)
	{
		const std::u32string u32str = AsUtf32(str);
		std::u32string result;
		for (const char32_t* c = u32str.data(); *c != U'\0'; ++c)
		{
			switch (*c)
			{
			case U'\n':
				result.append(U"<br>\n");
				break;
			case U'\t':
				result.append(U"&emsp;");
				break;
			case U' ':
				result.append(U"&ensp;");
				break;
			case U'\"':
				result.append(U"&quot;");
				break;
			case U'&':
				result.append(U"&amp;");
				break;
			case U'\'':
				result.append(U"&apos;");
				break;
			case U'<':
				result.append(U"&lt;");
				break;
			case U'>':
				result.append(U"&gt;");
				break;
			default:
				result.push_back(*c);
				break;
			}
		}

		return AsUtf8(result);
	}
}
