var $$$ = (function($){

	function filterEvent(e){
        e.stopPropagation();
        e.preventDefault();
        return false;
    }
 
	//disable text selection on page
	$('body').on('selectstart', filterEvent).on('drop', filterEvent).on('dragover', filterEvent).css({
        '-webkit-user-select':'none',
        '-moz-user-select':'none',
        '-ms-user-select':'none',
        '-o-user-select':'none',
        'user-select':'none'
        });

    var page = {};
    var body = document.getElementById("body");
    var video = document.getElementById("video");
    var data;
    
    page.play = function(){
       video.play();
    };

    page.pause = function(){
       video.pause();
    };

    page.paused = function(){
       return video.paused;
    };

    page.reset = function(){
       video.currentTime = 0;
    };

    page.capture = function(){
        var canvas = document.createElement("canvas");
        canvas.id = 'canvas';
        canvas.width = video.videoWidth;
        canvas.height = video.videoHeight;
        body.appendChild(canvas);
        video.setAttribute('crossorigin', 'anonymous');
        var context = canvas.getContext('2d');
        context.fillRect(0,0,canvas.width,canvas.height);
        context.drawImage(video, 0, 0, canvas.width, canvas.height);
        var dataURI = canvas.toDataURL('image/png');
        canvas.remove();
        return dataURI;
    };
           
    return page;
 
 })(jQuery);