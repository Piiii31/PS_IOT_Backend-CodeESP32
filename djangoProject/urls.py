from django.urls import path

from djangoProject.View import receive_sensor_data, login_view

urlpatterns = [
    path('api/data/', receive_sensor_data, name='receive_sensor_data'),
    path('', login_view, name='login_view'),
]
