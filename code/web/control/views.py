from django.shortcuts import render

# Create your views here.

from django.shortcuts import render_to_response
from django.http import HttpResponse
from urllib2 import urlopen

def imagen(request):
    url = "http://192.168.1.29:80/videostream.cgi?user=admin&amp;pwd=888888" 
    return HttpResponse(request, urlopen(url), mimetype="image/jpg")
