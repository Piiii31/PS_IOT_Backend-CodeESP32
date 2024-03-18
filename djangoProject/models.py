from django.db import models

class StorageArea(models.Model):
    node_id = models.CharField(max_length=100,unique=True)
    # Ajoutez d'autres champs si nécessaire

class Sample(models.Model):
    node_id = models.CharField(max_length=100)
    time_and_date = models.DateTimeField()
    sensor_type = models.CharField(max_length=100)
    value = models.FloatField()
    gps_location = models.CharField(max_length=100)
