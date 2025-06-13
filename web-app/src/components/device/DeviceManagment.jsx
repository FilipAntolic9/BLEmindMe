import { useState } from 'react';
import { useNotification } from '../../contexts/NotificationContext';
import { createDevice } from '../../services/DeviceService';
import Navbar from '../NavBar';

const DeviceManagement = () => {
    const [formData, setFormData] = useState({
        name: '',
        type: '',
        macAddress: '',
        deviceType: 'TAG' // default value
    });

    const { showNotification } = useNotification();

    const handleChange = (e) => {
        const { name, value } = e.target;
        setFormData(prevState => ({
            ...prevState,
            [name]: value
        }));
    };

    const handleSubmit = async (e) => {
        e.preventDefault();
        try {
            await createDevice(formData.name, formData.type, formData.macAddress, formData.deviceType);
            showNotification('Uređaj uspješno dodan!', 'success');
            setFormData({
                name: '',
                type: '',
                macAddress: '',
                deviceType: 'TAG'
            });
        } catch (err) {
            showNotification('Greška prilikom dodavanja uređaja: ' + err.message, 'error');
        }
    };

    return (
        <div className="min-h-screen bg-gray-100">
            <Navbar />
            <div className="max-w-md mx-auto mt-10 bg-white p-8 rounded-lg shadow">
                <h2 className="text-2xl font-bold mb-6 text-center">Dodaj novi uređaj</h2>
                <form onSubmit={handleSubmit} className="space-y-4">
                    <div>
                        <label className="block text-sm font-medium text-gray-700">Tip uređaja</label>
                        <select
                            name="deviceType"
                            value={formData.deviceType}
                            onChange={handleChange}
                            required
                            className="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 px-3"
                        >
                            <option value="TAG">Stvar za praćenje</option>
                            <option value="ESP">Uređaj za provjeru</option>
                        </select>
                    </div>
                    <div>
                        <label className="block text-sm font-medium text-gray-700">Naziv uređaja</label>
                        <input
                            type="text"
                            name="name"
                            value={formData.name}
                            onChange={handleChange}
                            required
                            className="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 px-3"
                        />
                    </div>
                    <div>
                        <label className="block text-sm font-medium text-gray-700">Opis</label>
                        <input
                            type="text"
                            name="type"
                            value={formData.type}
                            onChange={handleChange}
                            required
                            className="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 px-3"
                            placeholder="Kratki opis uređaja"
                        />
                    </div>
                    <div>
                        <label className="block text-sm font-medium text-gray-700">MAC adresa</label>
                        <input
                            type="text"
                            name="macAddress"
                            value={formData.macAddress}
                            onChange={handleChange}
                            required
                            className="mt-1 block w-full rounded-md border-gray-300 shadow-sm focus:border-blue-500 focus:ring-blue-500 px-3"
                            placeholder="XX:XX:XX:XX:XX:XX"
                        />
                    </div>
                    <button
                        type="submit"
                        className="w-full bg-blue-600 text-white py-2 px-4 rounded-md hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2"
                    >
                        Dodaj uređaj
                    </button>
                </form>
            </div>
        </div>
    );
};

export default DeviceManagement;