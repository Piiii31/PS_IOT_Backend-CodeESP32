from datetime import datetime
from django.shortcuts import redirect, render
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from .models import Sample, StorageArea
from django.contrib.auth.decorators import login_required
from django.middleware.csrf import get_token
from django.views.decorators.http import require_POST
from django.contrib.auth import authenticate, login
from rest_framework.decorators import api_view, permission_classes
from rest_framework.permissions import AllowAny
from rest_framework.authtoken.models import Token
from rest_framework.authentication import TokenAuthentication

@require_POST
@csrf_exempt
def receive_sensor_data(request):
    auth = TokenAuthentication()
    try:
        user, token = auth.authenticate(request)
    except Exception as e:
        return JsonResponse({'error': str(e)}, status=401)

    if user is not None:
        data = request.POST
        node_id = data.get('node_id')
        time_and_date_str = data.get('time_and_date')
        sensor_type = data.get('sensor_type')
        value_str = data.get('value')
        gps_location = data.get('gps_location')

        if None in [node_id, time_and_date_str, sensor_type, value_str, gps_location]:
            return JsonResponse({'error': 'Missing data in request'}, status=400)

        try:
            time_and_date = datetime.strptime(time_and_date_str, '%Y-%m-%d %H:%M:%S')
            value = float(value_str)
        except ValueError:
            return JsonResponse({'error': 'Invalid data format'}, status=400)

        sample = Sample.objects.create(node_id=node_id, time_and_date=time_and_date, sensor_type=sensor_type, value=value, gps_location=gps_location)
        storageArea = StorageArea.objects.create(node_id=node_id)

        return JsonResponse({'message': 'Data received successfully'}, status=201)
    else:
        return JsonResponse({'error': 'Unauthorized'}, status=401)

@csrf_exempt
@api_view(['POST'])
@permission_classes([AllowAny])
def login_view(request):
    if request.method == 'POST':
        username = request.POST.get('username')
        password = request.POST.get('password')
        print(password)
        print(username)

        user = authenticate(request, username=username, password=password)
        if user is not None:
            login(request, user)
            token, _ = Token.objects.get_or_create(user=user)  # Get the token for this user
            print(token)
            return JsonResponse({'token': str(token)}, status=200)  # Return the token
        else:
            return JsonResponse({'error': 'Invalid username or password'}, status=401)