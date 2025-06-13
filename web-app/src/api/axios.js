import axios from "axios";

const backendClient = axios.create({
    baseURL: import.meta.env.VITE_BACKEND_URL,
    headers: {
        'Content-Type': 'application/json',
    }
});

backendClient.interceptors.request.use(
    async (config) => {
        if (config.skipInterceptor) {
            return config;
        }

        const userJson = localStorage.getItem('user');

        if (userJson) {
            const user = JSON.parse(userJson);
            config.headers['X-Authorization'] = `Bearer ${user.accessToken}`;
        }
        return config;
    },
    (error) => Promise.reject(error)
);

export default backendClient;