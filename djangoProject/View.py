from datetime import datetime

from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from .models import Sample,StorageArea

@csrf_exempt
def receive_sensor_data(request):
    if request.method == 'POST':
        data = request.POST
        node_id = data.get('node_id')
        time_and_date_str = data.get('time_and_date')
        sensor_type = data.get('sensor_type')
        value_str = data.get('value')
        gps_location = data.get('gps_location')

        # Vérifier si des données obligatoires sont manquantes
        if None in [node_id, time_and_date_str, sensor_type, value_str, gps_location]:
            return JsonResponse({'error': 'Missing data in request'}, status=400)

        # Convertir la date et l'heure en objet datetime
        try:
            time_and_date = datetime.strptime(time_and_date_str, '%Y-%m-%d %H:%M:%S')
        except ValueError:
            return JsonResponse({'error': 'Invalid time_and_date format'}, status=400)

        # Convertir la valeur en float
        try:
            value = float(value_str)
        except ValueError:
            return JsonResponse({'error': 'Invalid value format'}, status=400)

        # Enregistrer les données dans la base de données
        sample = Sample.objects.create(node_id=node_id, time_and_date=time_and_date, sensor_type=sensor_type, value=value, gps_location=gps_location)
        storageArea=StorageArea.objects.create(node_id=node_id)

        return JsonResponse({'message': 'Data received successfully'}, status=201)
    else:
        return JsonResponse({'error': 'Method not allowed'}, status=405)
