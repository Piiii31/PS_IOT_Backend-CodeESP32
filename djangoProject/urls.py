from django.urls import path

from djangoProject.View import receive_sensor_data, login_view, get_storage_area_data

urlpatterns = [
    path('api/data/', receive_sensor_data, name='receive_sensor_data'),
    path('', login_view, name='login_view'),
    path('api/storagearea/<str:node_id>/', get_storage_area_data, name='get_storage_area_data'),
]
