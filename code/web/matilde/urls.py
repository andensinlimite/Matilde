from django.conf.urls import patterns, include, url

from django.contrib import admin
admin.autodiscover()

urlpatterns = patterns('',
    # Examples:
    # url(r'^$', 'matilde.views.home', name='home'),
    # url(r'^blog/', include('blog.urls')),

    url(r'^matilde/admin/', include(admin.site.urls)),
    url(r'^matilde/imagen$', 'control.views.imagen'),
)
